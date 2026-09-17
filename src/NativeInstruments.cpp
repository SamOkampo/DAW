#include "flowdaw/NativeInstruments.hpp"
#include "flowdaw/Midi.hpp"
#include "flowdaw/MusicalTime.hpp"
#include <algorithm>
#include <cmath>

namespace flowdaw {
namespace {
constexpr double pi=3.14159265358979323846;
float softClip(float x,float drive){if(drive<=0.0001f)return x;const float k=1.0f+std::clamp(drive,0.0f,1.0f)*8.0f;return std::tanh(x*k)/std::tanh(k);}
float osc(const std::string&type,double phase,double t,double freq,float tone){
    const double s=std::sin(phase);
    tone=std::clamp(tone,0.0f,1.0f);
    if(type=="flow_808"){
        const double pitchDrop=std::exp(-t*22.0);const double p=2.0*pi*freq*t*(1.0+0.12*pitchDrop);return static_cast<float>(std::sin(p));
    }
    if(type=="flow_bass"){
        const double saw=2.0*(phase/(2.0*pi)-std::floor(phase/(2.0*pi)+0.5));
        const double sub=std::sin(phase*0.5);return static_cast<float>((1.0-tone)*0.72*s+tone*0.36*saw+0.22*sub);
    }
    if(type=="flow_lead"){
        const double square=s>=0?1.0:-1.0;return static_cast<float>(0.72*s+0.18*tone*std::sin(phase*2.0)+0.10*tone*square);
    }
    return static_cast<float>(0.82*s+0.18*tone*std::sin(phase*2.0));
}
}

AudioBuffer renderNativeInstrumentNote(const InstrumentState& instrument,int midiPitch,float velocity,SampleIndex noteFrames,int sampleRate,double bpm){
    AudioBuffer out;out.sampleRate=sampleRate;out.channels=1;
    noteFrames=std::max<SampleIndex>(1,noteFrames);midiPitch=std::clamp(midiPitch,0,127);velocity=std::clamp(velocity,0.0f,1.5f);
    const SampleIndex attack=std::max<SampleIndex>(1,static_cast<SampleIndex>(std::llround(sampleRate*std::clamp(instrument.attackMs,0.0f,5000.0f)/1000.0)));
    const SampleIndex release=std::max<SampleIndex>(1,static_cast<SampleIndex>(std::llround(sampleRate*std::clamp(instrument.releaseMs,0.0f,10000.0f)/1000.0)));
    const SampleIndex delay=instrument.delayMix>0.0001f?MusicalTime::ticksToSamples(std::max<Tick>(1,instrument.delayTicks),std::max(20.0,bpm),sampleRate):0;
    const SampleIndex tail=release+(delay>0?delay*2:0);out.interleaved.assign(static_cast<std::size_t>(noteFrames+tail+2),0.0f);
    const double freq=midiNoteFrequency(midiPitch);double phase=0.0;const double inc=2.0*pi*freq/static_cast<double>(sampleRate);
    for(SampleIndex i=0;i<noteFrames+release;++i){
        const double t=static_cast<double>(i)/sampleRate;float env=1.0f;
        if(i<attack)env=static_cast<float>(i)/static_cast<float>(attack);
        if(i>=noteFrames){const float u=static_cast<float>(i-noteFrames)/static_cast<float>(release);env*=std::max(0.0f,1.0f-u);}
        float x=osc(instrument.type,phase,t,freq,instrument.tone)*env*velocity;x=softClip(x,instrument.drive);out.interleaved[static_cast<std::size_t>(i)]+=x;phase+=inc;if(phase>2.0*pi)phase-=2.0*pi;
    }
    const float mix=std::clamp(instrument.delayMix,0.0f,0.85f);
    if(delay>0&&mix>0){const auto dry=out.interleaved;for(int tap=1;tap<=2;++tap){const SampleIndex off=delay*tap;const float g=mix*std::pow(0.58f,static_cast<float>(tap-1));for(SampleIndex i=0;i<static_cast<SampleIndex>(dry.size())&&i+off<static_cast<SampleIndex>(out.interleaved.size());++i)out.interleaved[static_cast<std::size_t>(i+off)]+=dry[static_cast<std::size_t>(i)]*g;}}
    for(auto&x:out.interleaved)x=std::clamp(x,-1.2f,1.2f);
    return out;
}
}
