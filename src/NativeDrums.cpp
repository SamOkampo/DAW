#include "flowdaw/NativeDrums.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace flowdaw {
namespace { constexpr double pi=3.14159265358979323846; }
AudioBuffer makeNativeDrum(const std::string& key,int sr){
    if(sr<=0) throw std::invalid_argument("sampleRate");
    AudioBuffer a; a.sampleRate=sr;a.channels=1;
    double dur=key=="kick"?0.45:key=="snare"?0.32:0.12; a.interleaved.assign(static_cast<std::size_t>(sr*dur),0.0f);
    std::uint32_t rng=0x51A7BEEFu; double phase=0.0;
    for(std::size_t i=0;i<a.interleaved.size();++i){ double t=static_cast<double>(i)/sr; float v=0.0f;
        rng=rng*1664525u+1013904223u; float noise=(static_cast<int>((rng>>9)&0x7fffff)/4194303.5f)-1.0f;
        if(key=="kick"){
            double f=48.0+110.0*std::exp(-t*24.0); phase+=2*pi*f/sr; double env=std::exp(-t*9.0); v=static_cast<float>(0.92*std::sin(phase)*env + 0.12*noise*std::exp(-t*80.0));
        }else if(key=="snare"){
            double env=std::exp(-t*14.0); double tone=std::sin(2*pi*185.0*t)*std::exp(-t*20.0); float hp=noise; v=static_cast<float>((0.72*hp+0.28*tone)*env);
        }else if(key=="hat"){
            double env=std::exp(-t*42.0); float metallic=noise*0.72f + static_cast<float>(0.18*std::sin(2*pi*7600*t)+0.10*std::sin(2*pi*10100*t)); v=static_cast<float>(metallic*env*0.7);
        }else throw std::invalid_argument("unknown native drum: "+key);
        a.interleaved[i]=std::clamp(v,-1.0f,1.0f);
    }
    return a;
}
}
