#include "flowdaw/TimeStretch.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace flowdaw {
namespace {
float monoAt(const AudioBuffer& a,SampleIndex frame){
    if(frame<0||frame>=a.frames()) return 0.0f;
    float s=0.0f;
    for(int ch=0;ch<a.channels;++ch) s+=a.interleaved[static_cast<std::size_t>(frame*a.channels+ch)];
    return s/std::max(1,a.channels);
}
double similarity(const AudioBuffer& a,SampleIndex ref,SampleIndex candidate,int overlap){
    double dot=0.0,aa=0.0,bb=0.0;
    for(int i=0;i<overlap;i+=4){const double x=monoAt(a,ref+i),y=monoAt(a,candidate+i);dot+=x*y;aa+=x*x;bb+=y*y;}
    if(aa<1e-12||bb<1e-12) return -1.0;
    return dot/std::sqrt(aa*bb);
}
}
AudioBuffer timeStretchWsola(const AudioBuffer& input,double ratio){
    if(input.frames()<=0||input.channels<=0||input.sampleRate<=0) return input;
    if(ratio<0.5||ratio>2.0) throw std::invalid_argument("WSOLA ratio outside Phase-2 range 0.5..2.0");
    if(std::abs(ratio-1.0)<1e-6) return input;
    constexpr int window=2048,synthHop=1024,searchRadius=320;
    const int overlap=window-synthHop;
    const SampleIndex targetFrames=std::max<SampleIndex>(1,static_cast<SampleIndex>(std::llround(input.frames()*ratio)));
    AudioBuffer out; out.sampleRate=input.sampleRate; out.channels=input.channels; out.interleaved.assign(static_cast<std::size_t>(targetFrames*out.channels),0.0f);
    std::vector<float> weights(static_cast<std::size_t>(targetFrames),0.0f);
    std::vector<float> win(window); for(int i=0;i<window;++i) win[static_cast<std::size_t>(i)]=static_cast<float>(0.5-0.5*std::cos(2.0*3.14159265358979323846*i/(window-1)));
    const double analysisHop=static_cast<double>(synthHop)/ratio;
    SampleIndex prevAnalysis=0; int frameIndex=0;
    for(SampleIndex outStart=0;outStart<targetFrames;outStart+=synthHop,++frameIndex){
        SampleIndex expected=static_cast<SampleIndex>(std::llround(frameIndex*analysisHop));
        expected=std::clamp<SampleIndex>(expected,0,std::max<SampleIndex>(0,input.frames()-window));
        SampleIndex chosen=expected;
        if(frameIndex>0){
            const SampleIndex reference=std::min<SampleIndex>(prevAnalysis+synthHop,std::max<SampleIndex>(0,input.frames()-overlap));
            double best=-2.0;
            const SampleIndex lo=std::max<SampleIndex>(0,expected-searchRadius),hi=std::min<SampleIndex>(std::max<SampleIndex>(0,input.frames()-window),expected+searchRadius);
            for(SampleIndex c=lo;c<=hi;c+=4){double score=similarity(input,reference,c,overlap);if(score>best){best=score;chosen=c;}}
        }
        prevAnalysis=chosen;
        const int available=static_cast<int>(std::min<SampleIndex>(window,std::min(input.frames()-chosen,targetFrames-outStart)));
        if(available<=0) break;
        for(int i=0;i<available;++i){
            const float w=win[static_cast<std::size_t>(i)]; weights[static_cast<std::size_t>(outStart+i)]+=w;
            for(int ch=0;ch<input.channels;++ch){
                const auto src=static_cast<std::size_t>((chosen+i)*input.channels+ch),dst=static_cast<std::size_t>((outStart+i)*out.channels+ch);
                out.interleaved[dst]+=input.interleaved[src]*w;
            }
        }
    }
    for(SampleIndex f=0;f<targetFrames;++f){const float w=weights[static_cast<std::size_t>(f)];if(w>1e-5f)for(int ch=0;ch<out.channels;++ch)out.interleaved[static_cast<std::size_t>(f*out.channels+ch)]/=w;}
    return out;
}
AudioBuffer matchBpmWsola(const AudioBuffer& input,double sourceBpm,double targetBpm){
    if(sourceBpm<=0.0||targetBpm<=0.0) throw std::invalid_argument("BPM must be positive");
    return timeStretchWsola(input,sourceBpm/targetBpm);
}
}
