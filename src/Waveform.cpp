#include "flowdaw/Waveform.hpp"
#include <algorithm>
#include <limits>
namespace flowdaw {
std::vector<PeakPair> buildWaveform(const AudioBuffer& a,std::size_t columns){
    std::vector<PeakPair> out; if(columns==0 || a.frames()==0 || a.channels<=0) return out; out.resize(columns,{0,0});
    const auto n=static_cast<std::size_t>(a.frames());
    for(std::size_t x=0;x<columns;++x){
        std::size_t b=x*n/columns,e=std::max(b+1,(x+1)*n/columns); e=std::min(e,n); float lo=1.0f,hi=-1.0f;
        for(std::size_t i=b;i<e;++i){ float mono=0; for(int c=0;c<a.channels;++c) mono += a.interleaved[i*a.channels+c]/a.channels; lo=std::min(lo,mono); hi=std::max(hi,mono); }
        out[x]={lo,hi};
    }
    return out;
}
}
