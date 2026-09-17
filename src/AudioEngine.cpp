#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

#ifdef FLOWDAW_HAS_PORTAUDIO
extern "C" {
typedef int PaError; typedef void PaStream; typedef unsigned long PaSampleFormat; typedef unsigned long PaStreamCallbackFlags;
struct PaStreamCallbackTimeInfo { double inputBufferAdcTime; double currentTime; double outputBufferDacTime; };
typedef int PaStreamCallback(const void*,void*,unsigned long,const PaStreamCallbackTimeInfo*,PaStreamCallbackFlags,void*);
PaError Pa_Initialize(void); PaError Pa_Terminate(void);
PaError Pa_OpenDefaultStream(PaStream**,int,int,PaSampleFormat,double,unsigned long,PaStreamCallback*,void*);
PaError Pa_StartStream(PaStream*); PaError Pa_StopStream(PaStream*); PaError Pa_CloseStream(PaStream*);
const char* Pa_GetErrorText(PaError);
}
static constexpr PaError paNoError=0; static constexpr PaSampleFormat paFloat32=0x00000001UL;
#endif

namespace flowdaw {
namespace {
std::uint64_t eventHash(Id patternId,std::size_t lane,int step,int repeat,std::uint64_t salt){
    std::uint64_t x=patternId ^ salt ^ (static_cast<std::uint64_t>(lane+1)*0x9E3779B185EBCA87ULL) ^ (static_cast<std::uint64_t>(step+1)<<21) ^ (static_cast<std::uint64_t>(repeat+1)<<43);
    x^=x>>33; x*=0xff51afd7ed558ccdULL; x^=x>>33; x*=0xc4ceb9fe1a85ec53ULL; x^=x>>33; return x;
}
double deterministicUnit(Id patternId,std::size_t lane,int step,int repeat,std::uint64_t salt){
    return static_cast<double>(eventHash(patternId,lane,step,repeat,salt)&0xFFFFFFULL)/static_cast<double>(0x1000000ULL);
}
bool deterministicHit(float probability,Id patternId,std::size_t lane,int step,int repeat){
    if(probability>=1.0f) return true;
    if(probability<=0.0f) return false;
    return deterministicUnit(patternId,lane,step,repeat,0xA11CE5EEDULL)<probability;
}
double signedVariation(Id patternId,std::size_t lane,int step,int repeat,std::uint64_t salt){
    return deterministicUnit(patternId,lane,step,repeat,salt)*2.0-1.0;
}
}

AudioEngine::AudioEngine() = default;
AudioEngine::~AudioEngine(){ close(); }

bool AudioEngine::open(int sampleRate,unsigned long framesPerBuffer){
    sampleRate_=sampleRate; framesPerBuffer_=framesPerBuffer; error_.clear();
#ifdef FLOWDAW_HAS_PORTAUDIO
    auto e=Pa_Initialize(); if(e!=paNoError){ error_=Pa_GetErrorText(e); return false; }
    PaStream* s=nullptr;
    e=Pa_OpenDefaultStream(&s,0,2,paFloat32,static_cast<double>(sampleRate_),framesPerBuffer_,reinterpret_cast<PaStreamCallback*>(&AudioEngine::paCallback),this);
    if(e!=paNoError){ error_=Pa_GetErrorText(e); Pa_Terminate(); return false; }
    stream_=s; e=Pa_StartStream(s); if(e!=paNoError){ error_=Pa_GetErrorText(e); Pa_CloseStream(s); stream_=nullptr; Pa_Terminate(); return false; }
    return true;
#else
    error_="PortAudio runtime unavailable; offline renderer remains active"; return false;
#endif
}
void AudioEngine::close(){
#ifdef FLOWDAW_HAS_PORTAUDIO
    if(stream_){ Pa_StopStream(static_cast<PaStream*>(stream_)); Pa_CloseStream(static_cast<PaStream*>(stream_)); stream_=nullptr; Pa_Terminate(); }
#endif
}
std::string AudioEngine::lastError() const { return error_; }

void AudioEngine::publish(const Project& p){
    auto g=std::make_unique<Graph>(); g->master=p.master.volume;
    bool anySolo=false; for(auto const& t:p.tracks) anySolo=anySolo||t.mixer.solo;
    auto append=[&](SampleIndex start,SampleIndex sourceStart,SampleIndex length,float gain,float pan,std::shared_ptr<AudioBuffer> audio){
        if(!audio||audio->frames()==0||length<=0||gain<=0.0f) return;
        RenderClip r; r.start=start;r.sourceStart=sourceStart;r.length=length;r.gain=gain;r.pan=std::clamp(pan,-1.0f,1.0f);r.audio=std::move(audio);g->clips.push_back(std::move(r));
    };
    for(auto const& t:p.tracks){
        if(t.mixer.mute || (anySolo && !t.mixer.solo)) continue;
        for(auto const& c:t.clips){
            auto const* s=p.findSample(c.sampleId); if(!s||!s->audio) continue; const auto sourceStart=std::max<SampleIndex>(0,c.sourceStart); const auto requested=c.sourceLength>0?c.sourceLength:s->audio->frames()-sourceStart;
            append(MusicalTime::ticksToSamples(c.startTick,p.transport.bpm,sampleRate_),sourceStart,std::max<SampleIndex>(0,requested),c.gain*t.mixer.volume,t.mixer.pan,s->audio);
        }
        for(auto const& placement:t.patternClips){
            auto const* pat=p.findPattern(placement.patternId); if(!pat||pat->stepsPerBeat<=0) continue;
            const Tick stepTicks=kPPQ/pat->stepsPerBeat; const Tick patTicks=pat->lengthTicks();
            bool anyLaneSolo=false; for(auto const& lane:pat->lanes) anyLaneSolo=anyLaneSolo||lane.solo;
            const float swing=std::clamp(pat->swing,0.0f,1.0f), human=std::clamp(pat->humanize,0.0f,1.0f);
            for(int rep=0;rep<std::max(1,placement.repeats);++rep){
                for(std::size_t li=0;li<pat->lanes.size();++li){
                    auto const& lane=pat->lanes[li]; if(lane.mute||(anyLaneSolo&&!lane.solo)) continue;
                    auto const* s=p.findSample(lane.sampleId); if(!s||!s->audio) continue;
                    for(int step=0;step<std::min<int>(pat->stepCount,static_cast<int>(lane.steps.size()));++step){
                        auto const& ev=lane.steps[static_cast<std::size_t>(step)]; if(!ev.active||!deterministicHit(ev.probability,pat->id,li,step,rep)) continue;
                        const Tick swingTicks=(step%2==1)?static_cast<Tick>(std::llround(swing*static_cast<double>(stepTicks)*0.5)):0;
                        const Tick humanTicks=static_cast<Tick>(std::llround(signedVariation(pat->id,li,step,rep,0x71A1A6ULL)*human*static_cast<double>(stepTicks)*0.12));
                        const float humanVelocity=static_cast<float>(signedVariation(pat->id,li,step,rep,0xBEEFULL)*human*0.12);
                        Tick tick=placement.startTick+static_cast<Tick>(rep)*patTicks+static_cast<Tick>(step)*stepTicks+swingTicks+humanTicks+ev.microTicks; if(tick<0) tick=0;
                        const float velocity=std::clamp(ev.velocity+humanVelocity,0.0f,1.5f);
                        append(MusicalTime::ticksToSamples(tick,p.transport.bpm,sampleRate_),0,s->audio->frames(),velocity*lane.volume*t.mixer.volume,lane.pan+t.mixer.pan,s->audio);
                    }
                }
            }
        }
    }
    std::lock_guard lk(publishMutex_); Graph* raw=g.get(); retired_.push_back(std::move(g)); current_.store(raw,std::memory_order_release);
}
void AudioEngine::play(){ playing_.store(true,std::memory_order_release); }
void AudioEngine::pause(){ playing_.store(false,std::memory_order_release); }
void AudioEngine::stop(){ playing_.store(false,std::memory_order_release); playhead_.store(0,std::memory_order_release); }

int AudioEngine::process(float* out,unsigned long frames){
    std::fill(out,out+frames*2,0.0f); if(!playing_.load(std::memory_order_acquire)) return 0;
    auto* g=current_.load(std::memory_order_acquire); if(!g){ playhead_.fetch_add(static_cast<SampleIndex>(frames)); return 0; }
    const auto base=playhead_.load(std::memory_order_relaxed);
    for(auto const& c:g->clips){
        const double ratio=static_cast<double>(c.audio->sampleRate)/sampleRate_; const int ch=c.audio->channels; const float pan=std::clamp(c.pan,-1.0f,1.0f);
        const float lpan=std::sqrt((1.0f-pan)*0.5f), rpan=std::sqrt((1.0f+pan)*0.5f);
        for(unsigned long i=0;i<frames;++i){
            const auto timeline=base+static_cast<SampleIndex>(i); if(timeline<c.start) continue; const auto rel=timeline-c.start; const double srcPos=static_cast<double>(c.sourceStart)+static_cast<double>(rel)*ratio;
            if(srcPos>=static_cast<double>(c.sourceStart+c.length)||srcPos>=static_cast<double>(c.audio->frames()-1)) continue;
            const auto i0=static_cast<SampleIndex>(srcPos),i1=i0+1; const float frac=static_cast<float>(srcPos-i0);
            auto get=[&](SampleIndex idx,int cc){ return c.audio->interleaved[static_cast<std::size_t>(idx*ch+std::min(cc,ch-1))]; }; const float sl=get(i0,0)+(get(i1,0)-get(i0,0))*frac; const float sr=ch>1?get(i0,1)+(get(i1,1)-get(i0,1))*frac:sl;
            out[i*2]+=sl*c.gain*lpan*g->master; out[i*2+1]+=sr*c.gain*rpan*g->master;
        }
    }
    for(unsigned long i=0;i<frames*2;++i) out[i]=std::clamp(out[i],-1.0f,1.0f);
    playhead_.store(base+static_cast<SampleIndex>(frames),std::memory_order_release); return 0;
}
int AudioEngine::paCallback(const void*,void* output,unsigned long frames,const void*,unsigned long,void* user){ return static_cast<AudioEngine*>(user)->process(static_cast<float*>(output),frames); }
AudioBuffer AudioEngine::renderOffline(SampleIndex frames) const {
    AudioBuffer out; out.sampleRate=sampleRate_; out.channels=2; out.interleaved.assign(static_cast<std::size_t>(frames*2),0.0f); auto* g=current_.load(std::memory_order_acquire); if(!g) return out;
    for(auto const& c:g->clips){ const double ratio=static_cast<double>(c.audio->sampleRate)/sampleRate_; const int ch=c.audio->channels; const float pan=std::clamp(c.pan,-1.0f,1.0f),lpan=std::sqrt((1.0f-pan)*0.5f),rpan=std::sqrt((1.0f+pan)*0.5f);
        for(SampleIndex t=std::max<SampleIndex>(0,c.start);t<frames;++t){ const auto rel=t-c.start; const double srcPos=static_cast<double>(c.sourceStart)+static_cast<double>(rel)*ratio; if(srcPos>=static_cast<double>(c.sourceStart+c.length)||srcPos>=static_cast<double>(c.audio->frames()-1)) break; const auto i0=static_cast<SampleIndex>(srcPos),i1=i0+1; const float frac=static_cast<float>(srcPos-i0); auto get=[&](SampleIndex idx,int cc){return c.audio->interleaved[static_cast<std::size_t>(idx*ch+std::min(cc,ch-1))];}; const float sl=get(i0,0)+(get(i1,0)-get(i0,0))*frac,sr=ch>1?get(i0,1)+(get(i1,1)-get(i0,1))*frac:sl; out.interleaved[static_cast<std::size_t>(t*2)]+=sl*c.gain*lpan*g->master;out.interleaved[static_cast<std::size_t>(t*2+1)]+=sr*c.gain*rpan*g->master; }
    }
    for(auto& x:out.interleaved) x=std::clamp(x,-1.0f,1.0f);
    return out;
}
}
