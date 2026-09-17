#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/NativeInstruments.hpp"
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
std::uint64_t eventHash(Id patternId,std::size_t lane,int step,int repeat,std::uint64_t salt=0){
    std::uint64_t x=patternId ^ (static_cast<std::uint64_t>(lane+1)*0x9E3779B185EBCA87ULL) ^ (static_cast<std::uint64_t>(step+1)<<21) ^ (static_cast<std::uint64_t>(repeat+1)<<43) ^ salt;
    x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;
}
double deterministicUnit(Id p,std::size_t lane,int step,int repeat,std::uint64_t salt=0){return static_cast<double>(eventHash(p,lane,step,repeat,salt)&0xFFFFFFULL)/static_cast<double>(0x1000000ULL);}
bool deterministicHit(float probability,Id p,std::size_t lane,int step,int repeat){if(probability>=1.0f)return true;if(probability<=0.0f)return false;return deterministicUnit(p,lane,step,repeat)<probability;}
double signedVariation(Id p,std::size_t lane,int step,int repeat,std::uint64_t salt){return deterministicUnit(p,lane,step,repeat,salt)*2.0-1.0;}

void mixRange(float* out,unsigned long frames,const AudioBuffer& audio,SampleIndex sourceStart,SampleIndex sourceLength,SampleIndex outputOffset,float gain,float pan,int deviceRate){
    if(frames==0||sourceLength<=0||audio.frames()<=0||audio.channels<=0)return;
    const double ratio=static_cast<double>(audio.sampleRate)/deviceRate;const int ch=audio.channels;
    pan=std::clamp(pan,-1.0f,1.0f);const float lpan=std::sqrt((1.0f-pan)*0.5f),rpan=std::sqrt((1.0f+pan)*0.5f);
    for(unsigned long i=0;i<frames;++i){
        const SampleIndex rel=outputOffset+static_cast<SampleIndex>(i);if(rel<0)continue;
        const double srcPos=static_cast<double>(sourceStart)+static_cast<double>(rel)*ratio;
        if(srcPos>=static_cast<double>(sourceStart+sourceLength)||srcPos>=static_cast<double>(audio.frames()-1))break;
        const auto i0=static_cast<SampleIndex>(srcPos),i1=i0+1;const float frac=static_cast<float>(srcPos-i0);
        auto get=[&](SampleIndex idx,int cc){return audio.interleaved[static_cast<std::size_t>(idx*ch+std::min(cc,ch-1))];};
        const float sl=get(i0,0)+(get(i1,0)-get(i0,0))*frac;const float sr=ch>1?get(i0,1)+(get(i1,1)-get(i0,1))*frac:sl;
        out[i*2]+=sl*gain*lpan;out[i*2+1]+=sr*gain*rpan;
    }
}
}

AudioEngine::AudioEngine()=default;
AudioEngine::~AudioEngine(){close();}
bool AudioEngine::open(int sampleRate,unsigned long framesPerBuffer){
    sampleRate_=sampleRate;framesPerBuffer_=framesPerBuffer;error_.clear();
#ifdef FLOWDAW_HAS_PORTAUDIO
    auto e=Pa_Initialize();if(e!=paNoError){error_=Pa_GetErrorText(e);return false;}PaStream* s=nullptr;
    e=Pa_OpenDefaultStream(&s,0,2,paFloat32,static_cast<double>(sampleRate_),framesPerBuffer_,reinterpret_cast<PaStreamCallback*>(&AudioEngine::paCallback),this);
    if(e!=paNoError){error_=Pa_GetErrorText(e);Pa_Terminate();return false;}stream_=s;e=Pa_StartStream(s);if(e!=paNoError){error_=Pa_GetErrorText(e);Pa_CloseStream(s);stream_=nullptr;Pa_Terminate();return false;}return true;
#else
    error_="PortAudio runtime unavailable; offline renderer remains active";return false;
#endif
}
void AudioEngine::close(){
#ifdef FLOWDAW_HAS_PORTAUDIO
    if(stream_){Pa_StopStream(static_cast<PaStream*>(stream_));Pa_CloseStream(static_cast<PaStream*>(stream_));stream_=nullptr;Pa_Terminate();}
#endif
}
std::string AudioEngine::lastError() const{return error_;}

void AudioEngine::publish(const Project& p){
    auto g=std::make_unique<Graph>();g->master=p.master.volume;bool anySolo=false;for(auto const&t:p.tracks)anySolo=anySolo||t.mixer.solo;
    auto append=[&](SampleIndex start,SampleIndex sourceStart,SampleIndex length,float gain,float pan,std::shared_ptr<AudioBuffer> audio,int chokeGroup){if(!audio||audio->frames()==0||length<=0||gain<=0.0f)return;RenderClip r;r.start=start;r.sourceStart=sourceStart;r.length=length;r.gain=gain;r.pan=std::clamp(pan,-1.0f,1.0f);r.chokeGroup=std::max(0,chokeGroup);r.audio=std::move(audio);g->clips.push_back(std::move(r));};
    for(auto const&t:p.tracks){
        if(t.mixer.mute||(anySolo&&!t.mixer.solo))continue;
        const std::size_t trackClipBase=g->clips.size();
        for(auto const&c:t.clips){auto const*s=p.findSample(c.sampleId);if(!s||!s->audio)continue;const auto sourceStart=std::max<SampleIndex>(0,c.sourceStart);const auto requested=c.sourceLength>0?c.sourceLength:s->audio->frames()-sourceStart;append(MusicalTime::ticksToSamples(c.startTick,p.transport.bpm,sampleRate_),sourceStart,std::max<SampleIndex>(0,requested),c.gain*t.mixer.volume,t.mixer.pan,s->audio,0);}
        for(auto const&placement:t.patternClips){
            auto const*pat=p.findPattern(placement.patternId);if(!pat||pat->stepsPerBeat<=0)continue;
            const Tick stepTicks=kPPQ/pat->stepsPerBeat,patTicks=pat->lengthTicks();
            bool anyLaneSolo=false;for(auto const&lane:pat->lanes)anyLaneSolo=anyLaneSolo||lane.solo;
            const float swing=std::clamp(pat->swing,0.0f,1.0f),human=std::clamp(pat->humanize,0.0f,1.0f);
            for(int rep=0;rep<std::max(1,placement.repeats);++rep){
                for(std::size_t li=0;li<pat->lanes.size();++li){
                    auto const&lane=pat->lanes[li];if(lane.mute||(anyLaneSolo&&!lane.solo))continue;auto const*s=p.findSample(lane.sampleId);if(!s||!s->audio)continue;
                    for(int step=0;step<std::min<int>(pat->stepCount,static_cast<int>(lane.steps.size()));++step){
                        auto const&ev=lane.steps[static_cast<std::size_t>(step)];if(!ev.active||!deterministicHit(ev.probability,pat->id,li,step,rep))continue;
                        const Tick swingTicks=(step%2==1)?static_cast<Tick>(std::llround(swing*stepTicks*0.5)):0;const Tick humanTicks=static_cast<Tick>(std::llround(signedVariation(pat->id,li,step,rep,0xA51ULL)*human*stepTicks*0.12));const float humanVelocity=static_cast<float>(signedVariation(pat->id,li,step,rep,0xB73ULL)*human*0.12);
                        Tick tick=placement.startTick+static_cast<Tick>(rep)*patTicks+static_cast<Tick>(step)*stepTicks+swingTicks+humanTicks+ev.microTicks;if(tick<0)tick=0;const float velocity=std::clamp(ev.velocity+humanVelocity,0.0f,1.5f);
                        append(MusicalTime::ticksToSamples(tick,p.transport.bpm,sampleRate_),0,s->audio->frames(),velocity*lane.volume*t.mixer.volume,lane.pan+t.mixer.pan,s->audio,0);
                    }
                }
                if(pat->instrument.enabled){
                    for(auto const&note:pat->midiNotes){
                        if(note.lengthTicks<=0||note.velocity<=0.0f)continue;
                        const Tick tick=std::max<Tick>(0,placement.startTick+static_cast<Tick>(rep)*patTicks+note.startTick);
                        const SampleIndex noteFrames=std::max<SampleIndex>(1,MusicalTime::ticksToSamples(note.lengthTicks,p.transport.bpm,sampleRate_));
                        auto synth=std::make_shared<AudioBuffer>(renderNativeInstrumentNote(pat->instrument,note.pitch,note.velocity,noteFrames,sampleRate_,p.transport.bpm));
                        const SampleIndex synthFrames=synth->frames();
                        append(MusicalTime::ticksToSamples(tick,p.transport.bpm,sampleRate_),0,synthFrames,std::clamp(pat->instrument.gain,0.0f,2.0f)*t.mixer.volume,pat->instrument.pan+t.mixer.pan,std::move(synth),0);
                    }
                }
                for(auto const&ev:pat->chopEvents){
                    auto const*s=p.findSample(ev.sampleId);if(!s||!s->audio)continue;
                    auto slice=std::find_if(s->slices.begin(),s->slices.end(),[&](auto const&sl){return sl.id==ev.sliceId;});if(slice==s->slices.end())continue;
                    const auto sourceStart=std::clamp<SampleIndex>(slice->startFrame,0,s->audio->frames());const auto sourceEnd=std::clamp<SampleIndex>(slice->endFrame,sourceStart,s->audio->frames());if(sourceEnd<=sourceStart)continue;
                    Tick tick=placement.startTick+static_cast<Tick>(rep)*patTicks+ev.tick;if(tick<0)tick=0;
                    const SampleIndex startSamples=MusicalTime::ticksToSamples(tick,p.transport.bpm,sampleRate_);
                    const int choke=std::max(0,slice->chokeGroup);
                    if(choke>0){
                        for(std::size_t idx=g->clips.size();idx>trackClipBase;--idx){
                            auto&previous=g->clips[idx-1];if(previous.chokeGroup!=choke||previous.start>startSamples||!previous.audio)continue;
                            const SampleIndex outputGap=std::max<SampleIndex>(0,startSamples-previous.start);
                            const double sourcePerDevice=static_cast<double>(previous.audio->sampleRate)/static_cast<double>(sampleRate_);
                            const SampleIndex cutFrames=std::max<SampleIndex>(1,static_cast<SampleIndex>(std::llround(static_cast<double>(outputGap)*sourcePerDevice)));
                            previous.length=std::min(previous.length,cutFrames);break;
                        }
                    }
                    const float gain=std::clamp(ev.velocity,0.0f,1.5f)*std::clamp(slice->gain,0.0f,2.0f)*t.mixer.volume;
                    const float pan=ev.pan+slice->pan+t.mixer.pan;
                    append(startSamples,sourceStart,sourceEnd-sourceStart,gain,pan,s->audio,choke);
                }
            }
        }
    }
    std::lock_guard lk(publishMutex_);Graph*raw=g.get();retired_.push_back(std::move(g));current_.store(raw,std::memory_order_release);
}
void AudioEngine::play(){playing_.store(true,std::memory_order_release);}void AudioEngine::pause(){playing_.store(false,std::memory_order_release);}void AudioEngine::stop(){playing_.store(false,std::memory_order_release);playhead_.store(0,std::memory_order_release);}

bool AudioEngine::triggerPreview(std::shared_ptr<AudioBuffer> audio,SampleIndex sourceStart,SampleIndex sourceLength,float gain,float pan,int chokeGroup){
    if(!audio||audio->frames()<2)return false;
    sourceStart=std::clamp<SampleIndex>(sourceStart,0,audio->frames()-1);
    sourceLength=std::min<SampleIndex>(std::max<SampleIndex>(1,sourceLength),audio->frames()-sourceStart);
    {std::lock_guard lk(previewLifetimeMutex_);bool seen=false;for(auto const&p:previewKeepAlive_)if(p.get()==audio.get()){seen=true;break;}if(!seen)previewKeepAlive_.push_back(audio);}
    const std::uint32_t w=previewWrite_.load(std::memory_order_relaxed);
    const std::uint32_t r=previewRead_.load(std::memory_order_acquire);
    const std::uint32_t next=static_cast<std::uint32_t>((w+1U)%static_cast<std::uint32_t>(kPreviewQueueSize));
    if(next==r)return false;
    previewQueue_[w]={audio.get(),sourceStart,sourceLength,gain,std::clamp(pan,-1.0f,1.0f),std::max(0,chokeGroup),false};
    previewWrite_.store(next,std::memory_order_release);
    return true;
}
void AudioEngine::stopPreviews(){
    const std::uint32_t w=previewWrite_.load(std::memory_order_relaxed);
    const std::uint32_t r=previewRead_.load(std::memory_order_acquire);
    const std::uint32_t next=static_cast<std::uint32_t>((w+1U)%static_cast<std::uint32_t>(kPreviewQueueSize));
    if(next==r)return;
    previewQueue_[w]={nullptr,0,0,1,0,0,true};
    previewWrite_.store(next,std::memory_order_release);
}
void AudioEngine::consumePreviewCommands(){
    auto r=previewRead_.load(std::memory_order_relaxed);const auto w=previewWrite_.load(std::memory_order_acquire);
    while(r!=w){
        const auto cmd=previewQueue_[r];
        if(cmd.stopAll){for(auto&v:previewVoices_)v.active=false;}
        else if(cmd.audio){
            if(cmd.chokeGroup>0)for(auto&v:previewVoices_)if(v.active&&v.chokeGroup==cmd.chokeGroup)v.active=false;
            PreviewVoice*slot=nullptr;for(auto&v:previewVoices_)if(!v.active){slot=&v;break;}if(!slot)slot=&previewVoices_[0];
            *slot={cmd.audio,cmd.start,cmd.length,0,cmd.gain,cmd.pan,cmd.chokeGroup,true};
        }
        r=static_cast<std::uint32_t>((r+1U)%static_cast<std::uint32_t>(kPreviewQueueSize));
    }
    previewRead_.store(r,std::memory_order_release);
}
void AudioEngine::mixPreviewVoices(float*out,unsigned long frames){
    for(auto&v:previewVoices_){if(!v.active||!v.audio)continue;const auto before=v.position;mixRange(out,frames,*v.audio,v.start,v.length,before,v.gain,v.pan,sampleRate_);v.position+=static_cast<SampleIndex>(frames);const double ratio=static_cast<double>(v.audio->sampleRate)/sampleRate_;if(static_cast<double>(v.position)*ratio>=static_cast<double>(v.length))v.active=false;}
}

int AudioEngine::process(float*out,unsigned long frames){
    std::fill(out,out+frames*2,0.0f);consumePreviewCommands();const bool transport=playing_.load(std::memory_order_acquire);auto*g=current_.load(std::memory_order_acquire);const auto base=playhead_.load(std::memory_order_relaxed);
    if(transport&&g){for(auto const&c:g->clips){if(base+static_cast<SampleIndex>(frames)<=c.start)continue;const SampleIndex localOffset=std::max<SampleIndex>(0,base-c.start);const unsigned long outOffset=base<c.start?static_cast<unsigned long>(c.start-base):0;if(outOffset>=frames)continue;mixRange(out+outOffset*2,frames-outOffset,*c.audio,c.sourceStart,c.length,localOffset,c.gain*g->master,c.pan,sampleRate_);}}
    mixPreviewVoices(out,frames);for(unsigned long i=0;i<frames*2;++i)out[i]=std::clamp(out[i],-1.0f,1.0f);if(transport)playhead_.store(base+static_cast<SampleIndex>(frames),std::memory_order_release);return 0;
}
int AudioEngine::paCallback(const void*,void*output,unsigned long frames,const void*,unsigned long,void*user){return static_cast<AudioEngine*>(user)->process(static_cast<float*>(output),frames);}

AudioBuffer AudioEngine::renderOffline(SampleIndex frames)const{
    AudioBuffer out;out.sampleRate=sampleRate_;out.channels=2;out.interleaved.assign(static_cast<std::size_t>(std::max<SampleIndex>(0,frames)*2),0.0f);auto*g=current_.load(std::memory_order_acquire);if(!g)return out;
    for(auto const&c:g->clips){const SampleIndex start=std::max<SampleIndex>(0,c.start);if(start>=frames)continue;const unsigned long count=static_cast<unsigned long>(frames-start);mixRange(out.interleaved.data()+start*2,count,*c.audio,c.sourceStart,c.length,std::max<SampleIndex>(0,-c.start),c.gain*g->master,c.pan,sampleRate_);}for(auto&x:out.interleaved)x=std::clamp(x,-1.0f,1.0f);return out;
}
AudioBuffer AudioEngine::renderDeviceBlockForTest(SampleIndex frames){AudioBuffer out;out.sampleRate=sampleRate_;out.channels=2;out.interleaved.assign(static_cast<std::size_t>(std::max<SampleIndex>(0,frames)*2),0.0f);if(frames>0)process(out.interleaved.data(),static_cast<unsigned long>(frames));return out;}
}
