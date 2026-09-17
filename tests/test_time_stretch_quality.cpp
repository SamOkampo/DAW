#include "flowdaw/TimeStretch.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

namespace {
constexpr double kPi=3.14159265358979323846;
void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}

AudioBuffer sine(double hz,double seconds,int sampleRate=48000){
    AudioBuffer a;a.sampleRate=sampleRate;a.channels=1;
    const auto frames=static_cast<SampleIndex>(std::llround(seconds*sampleRate));
    a.interleaved.resize(static_cast<std::size_t>(frames));
    for(SampleIndex i=0;i<frames;++i)a.interleaved[static_cast<std::size_t>(i)]=static_cast<float>(0.65*std::sin(2.0*kPi*hz*i/sampleRate));
    return a;
}

AudioBuffer impulses(double seconds,int sampleRate=48000){
    AudioBuffer a;a.sampleRate=sampleRate;a.channels=1;
    const auto frames=static_cast<SampleIndex>(std::llround(seconds*sampleRate));
    a.interleaved.assign(static_cast<std::size_t>(frames),0.0f);
    for(SampleIndex i=0;i<frames;i+=sampleRate/4){
        for(int k=0;k<48&&i+k<frames;++k)a.interleaved[static_cast<std::size_t>(i+k)]=static_cast<float>(std::exp(-k/8.0));
    }
    return a;
}

double rms(const AudioBuffer& a){
    if(a.interleaved.empty())return 0.0;
    double e=0.0;for(float s:a.interleaved)e+=static_cast<double>(s)*s;
    return std::sqrt(e/a.interleaved.size());
}

double estimateZeroCrossingHz(const AudioBuffer& a){
    if(a.frames()<2||a.sampleRate<=0)return 0.0;
    int crossings=0;
    for(SampleIndex i=1;i<a.frames();++i){
        const float p=a.interleaved[static_cast<std::size_t>(i-1)],q=a.interleaved[static_cast<std::size_t>(i)];
        if((p<=0.0f&&q>0.0f)||(p>=0.0f&&q<0.0f))++crossings;
    }
    return crossings*0.5*a.sampleRate/static_cast<double>(a.frames());
}

double peak(const AudioBuffer& a){double p=0.0;for(float s:a.interleaved)p=std::max(p,std::abs(static_cast<double>(s)));return p;}
}

int main(){
    try{
        const auto tone=sine(440.0,2.0);
        for(double ratio:{0.5,0.67,0.8,1.25,1.5,2.0}){
            const auto out=timeStretchWsola(tone,ratio);
            const auto expected=static_cast<SampleIndex>(std::llround(tone.frames()*ratio));
            require(std::llabs(out.frames()-expected)<=1,"WSOLA duration mismatch");
            const double hz=estimateZeroCrossingHz(out);
            require(std::abs(hz-440.0)<8.0,"WSOLA pitch drift exceeds tolerance");
            require(rms(out)>0.10,"WSOLA output unexpectedly quiet");
            require(peak(out)<1.5,"WSOLA produced unstable peak");
        }

        const auto transients=impulses(2.0);
        for(double ratio:{0.6,0.85,1.2,1.7}){
            const auto out=timeStretchWsola(transients,ratio);
            require(out.frames()==static_cast<SampleIndex>(std::llround(transients.frames()*ratio)),"transient stretch duration mismatch");
            require(peak(out)>0.45,"transients were excessively smeared/attenuated");
            require(rms(out)>0.005,"transient stretch lost too much energy");
        }

        const auto a=timeStretchWsola(tone,1.37),b=timeStretchWsola(tone,1.37);
        require(a.interleaved==b.interleaved,"WSOLA must be deterministic");

        bool lowRejected=false,highRejected=false;
        try{(void)timeStretchWsola(tone,0.49);}catch(const std::invalid_argument&){lowRejected=true;}
        try{(void)timeStretchWsola(tone,2.01);}catch(const std::invalid_argument&){highRejected=true;}
        require(lowRejected&&highRejected,"unsupported stretch ratios must be rejected explicitly");

        const auto bpmMatched=matchBpmWsola(tone,82.0,90.0);
        const auto expected=static_cast<SampleIndex>(std::llround(tone.frames()*(82.0/90.0)));
        require(std::llabs(bpmMatched.frames()-expected)<=1,"BPM matching ratio mismatch");

        std::cout<<"FLOWDAW stretch quality tests passed\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW stretch quality test failed: "<<e.what()<<"\n";
        return 1;
    }
}
