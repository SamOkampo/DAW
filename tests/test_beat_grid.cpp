#include "flowdaw/SampleAnalysis.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}

static AudioBuffer accentedClicks(double bpm,int beats,SampleIndex offset,int sr=48000){
    AudioBuffer a;a.sampleRate=sr;a.channels=1;const auto interval=static_cast<SampleIndex>(std::llround(sr*60.0/bpm));a.interleaved.assign(static_cast<std::size_t>(offset+interval*beats+2000),0.0f);
    for(int b=0;b<beats;++b){const auto start=offset+interval*b;const float amp=b%4==0?1.0f:.42f;for(int i=0;i<600;++i){const auto f=start+i;if(f>=a.frames())break;a.interleaved[static_cast<std::size_t>(f)]=amp*static_cast<float>(std::exp(-i/70.0));}}
    return a;
}

int main(){
    try{
        constexpr double bpm=90.0;constexpr SampleIndex offset=5000;auto audio=accentedClicks(bpm,28,offset);
        auto grid=estimateBeatGrid(audio,70,120);require(grid.valid(),"beat grid should be valid");require(std::abs(grid.bpm-bpm)<2.0,"beat grid BPM");require(std::llabs(grid.firstBeatFrame-offset)<1800,"first beat phase should follow pickup offset");require(std::llabs(grid.downbeatFrame-offset)<1800,"accented beat should be downbeat candidate");require(grid.downbeatConfidence>0.2,"downbeat confidence should reflect accent pattern");
        auto beats=makeBeatSlices(audio,grid,1);require(beats.size()>20,"beat slices should cover track");require(beats.front().startFrame==0&&beats.back().endFrame==audio.frames(),"beat slices cover full source");
        auto bars=makeBeatSlices(audio,grid,4);require(bars.size()>=7&&bars.size()<10,"bar slicing should group four beats");require(bars.front().startFrame==0&&bars.back().endFrame==audio.frames(),"bar slices cover full source");
        std::cout<<"FLOWDAW BeatGrid tests: PASS\n";return 0;
    }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
