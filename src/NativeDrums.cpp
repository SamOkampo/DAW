#include "flowdaw/NativeDrums.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace flowdaw {
namespace { constexpr double pi=3.14159265358979323846; }
AudioBuffer makeNativeDrum(const std::string& key,int sr){
    if(sr<=0) throw std::invalid_argument("sampleRate");
    double dur=0.18; if(key=="kick")dur=0.45;else if(key=="snare"||key=="clap")dur=0.34;else if(key=="openhat")dur=0.52;else if(key=="rim")dur=0.16;else if(key=="perc")dur=0.24;else if(key!="hat")throw std::invalid_argument("unknown native drum: "+key);
    AudioBuffer a;a.sampleRate=sr;a.channels=1;a.interleaved.assign(static_cast<std::size_t>(sr*dur),0.0f);
    std::uint32_t rng=0x51A7BEEFu;double phase=0.0;float lastNoise=0.0f;
    for(std::size_t i=0;i<a.interleaved.size();++i){double t=static_cast<double>(i)/sr;float v=0.0f;rng=rng*1664525u+1013904223u;float noise=(static_cast<int>((rng>>9)&0x7fffff)/4194303.5f)-1.0f;float hp=noise-lastNoise*0.72f;lastNoise=noise;
        if(key=="kick"){double f=48.0+110.0*std::exp(-t*24.0);phase+=2*pi*f/sr;v=static_cast<float>(0.92*std::sin(phase)*std::exp(-t*9.0)+0.12*noise*std::exp(-t*80.0));}
        else if(key=="snare"){v=static_cast<float>((0.72*hp+0.28*std::sin(2*pi*185*t))*std::exp(-t*14.0));}
        else if(key=="hat"||key=="openhat"){double decay=key=="openhat"?9.0:42.0;float metallic=hp*0.72f+static_cast<float>(0.18*std::sin(2*pi*7600*t)+0.10*std::sin(2*pi*10100*t));v=static_cast<float>(metallic*std::exp(-t*decay)*0.7);}
        else if(key=="clap"){double bursts=std::exp(-std::fmod(t,0.018)*85.0)*(t<0.075?1.0:0.45);v=static_cast<float>(hp*bursts*std::exp(-t*10.5)*0.78);}
        else if(key=="rim"){v=static_cast<float>((0.72*std::sin(2*pi*1750*t)+0.28*hp)*std::exp(-t*34.0)*0.72);}
        else if(key=="perc"){v=static_cast<float>((0.55*std::sin(2*pi*510*t)+0.30*std::sin(2*pi*760*t)+0.15*hp)*std::exp(-t*18.0)*0.75);}
        a.interleaved[i]=std::clamp(v,-1.0f,1.0f);
    }
    return a;
}
}
