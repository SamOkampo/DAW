#include "flowdaw/SampleAnalysis.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace flowdaw {
namespace {
std::vector<double> onsetEnvelope(const AudioBuffer& audio,int hop=256,int window=1024){
    if(audio.frames()<=0||audio.channels<=0||audio.sampleRate<=0) return {};
    const auto frames=audio.frames();
    const std::size_t count=static_cast<std::size_t>((frames+hop-1)/hop);
    std::vector<double> rms(count,0.0),novelty(count,0.0);
    for(std::size_t i=0;i<count;++i){
        const SampleIndex start=static_cast<SampleIndex>(i)*hop;
        const SampleIndex end=std::min<SampleIndex>(frames,start+window);
        double sum=0.0; SampleIndex n=0;
        for(SampleIndex f=start;f<end;++f){
            double mono=0.0;
            for(int ch=0;ch<audio.channels;++ch) mono+=audio.interleaved[static_cast<std::size_t>(f*audio.channels+ch)];
            mono/=audio.channels; sum+=mono*mono; ++n;
        }
        if(n>0) rms[i]=std::sqrt(sum/static_cast<double>(n));
        novelty[i]=std::max(0.0,rms[i]-(i?rms[i-1]:0.0));
    }
    double maxv=0.0; for(double v:novelty) maxv=std::max(maxv,v);
    if(maxv>0.0) for(double& v:novelty) v/=maxv;
    return novelty;
}
}

std::vector<SampleIndex> detectTransients(const AudioBuffer& audio,double sensitivity,double minSpacingSeconds){
    constexpr int hop=256;
    auto env=onsetEnvelope(audio,hop,1024); if(env.size()<3) return {};
    const double mean=std::accumulate(env.begin(),env.end(),0.0)/static_cast<double>(env.size());
    double variance=0.0; for(double v:env){const double d=v-mean;variance+=d*d;} variance/=static_cast<double>(env.size());
    const double threshold=std::clamp(mean+sensitivity*std::sqrt(variance),0.03,0.85);
    const SampleIndex minSpacing=std::max<SampleIndex>(1,static_cast<SampleIndex>(audio.sampleRate*minSpacingSeconds));
    std::vector<SampleIndex> out; SampleIndex last=-minSpacing;
    for(std::size_t i=1;i+1<env.size();++i){
        if(env[i]<threshold||env[i]<env[i-1]||env[i]<env[i+1]) continue;
        const SampleIndex frame=std::min<SampleIndex>(static_cast<SampleIndex>(i)*hop,std::max<SampleIndex>(0,audio.frames()-1));
        if(frame-last>=minSpacing){out.push_back(frame);last=frame;}
    }
    return out;
}

BpmEstimate detectBpm(const AudioBuffer& audio,double minBpm,double maxBpm){
    constexpr int hop=256;
    if(minBpm<=0||maxBpm<=minBpm) return {};
    auto env=onsetEnvelope(audio,hop,1024); if(env.size()<8) return {};
    const double hopSeconds=static_cast<double>(hop)/audio.sampleRate;
    const int minLag=std::max(1,static_cast<int>(std::floor(60.0/(maxBpm*hopSeconds))));
    const int maxLag=std::min<int>(static_cast<int>(env.size())-2,static_cast<int>(std::ceil(60.0/(minBpm*hopSeconds))));
    double best=-1.0; int bestLag=0;
    for(int lag=minLag;lag<=maxLag;++lag){
        double dot=0.0,a2=0.0,b2=0.0;
        for(std::size_t i=static_cast<std::size_t>(lag);i<env.size();++i){const double a=env[i],b=env[i-static_cast<std::size_t>(lag)];dot+=a*b;a2+=a*a;b2+=b*b;}
        if(a2<=1e-12||b2<=1e-12) continue;
        const double corr=dot/std::sqrt(a2*b2);
        const double bpm=60.0/(lag*hopSeconds);
        const double tieBias=1.0-0.00015*(bpm-minBpm);
        const double score=corr*tieBias;
        if(score>best){best=score;bestLag=lag;}
    }
    if(bestLag==0||best<0.05) return {};
    BpmEstimate result; result.bpm=60.0/(bestLag*hopSeconds); result.confidence=std::clamp(best,0.0,1.0); return result;
}

std::vector<SliceRange> makeEqualSlices(const AudioBuffer& audio,int count){
    if(count<=0) throw std::invalid_argument("slice count");
    std::vector<SliceRange> out; if(audio.frames()<=0) return out; out.reserve(static_cast<std::size_t>(count));
    for(int i=0;i<count;++i){out.push_back({audio.frames()*i/count,audio.frames()*(i+1)/count});}
    return out;
}
}
