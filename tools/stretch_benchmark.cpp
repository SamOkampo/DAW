#include "flowdaw/TimeStretch.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace flowdaw;

static AudioBuffer transientRich(double seconds=8.0,int sr=48000){
    AudioBuffer a;a.sampleRate=sr;a.channels=1;const auto frames=static_cast<SampleIndex>(std::llround(seconds*sr));a.interleaved.resize(static_cast<std::size_t>(frames));
    for(SampleIndex i=0;i<frames;++i){
        const double t=static_cast<double>(i)/sr;
        double x=.18*std::sin(2.0*3.141592653589793*110.0*t)+.08*std::sin(2.0*3.141592653589793*330.0*t);
        const auto beat=i%(sr/2);if(beat<700)x+=.8*std::exp(-static_cast<double>(beat)/90.0);
        a.interleaved[static_cast<std::size_t>(i)]=static_cast<float>(std::clamp(x,-1.0,1.0));
    }
    return a;
}

int main(){
    auto source=transientRich();
    const std::vector<double> ratios{.5,.75,1.0,1.25,1.5,2.0};
    std::cout<<"FLOWDAW WSOLA benchmark | source="<<std::fixed<<std::setprecision(2)<<static_cast<double>(source.frames())/source.sampleRate<<" s @ "<<source.sampleRate<<" Hz\n";
    std::cout<<"ratio,output_seconds,processing_ms,realtime_factor\n";
    for(double ratio:ratios){
        const auto begin=std::chrono::steady_clock::now();
        auto output=timeStretchWsola(source,ratio);
        const auto end=std::chrono::steady_clock::now();
        const double ms=std::chrono::duration<double,std::milli>(end-begin).count();
        const double outputSeconds=static_cast<double>(output.frames())/output.sampleRate;
        const double realtimeFactor=outputSeconds>0?(ms/1000.0)/outputSeconds:0.0;
        std::cout<<std::setprecision(3)<<ratio<<","<<outputSeconds<<","<<ms<<","<<realtimeFactor<<"\n";
        if(output.frames()<=0||!std::isfinite(realtimeFactor))return 2;
    }
    return 0;
}
