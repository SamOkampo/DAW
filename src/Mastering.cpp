#include "flowdaw/Mastering.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace flowdaw {
namespace {

constexpr double kPi=3.1415926535897932384626433832795;
constexpr double kLoudnessOffset=-0.691;
constexpr double kAbsoluteGateLufs=-70.0;

struct Biquad {
    double b0=1.0,b1=0.0,b2=0.0,a1=0.0,a2=0.0;
    double z1=0.0,z2=0.0;
    double process(double x) noexcept {
        const double y=b0*x+z1;
        z1=b1*x-a1*y+z2;
        z2=b2*x-a2*y;
        if(std::abs(z1)<1.0e-30)z1=0.0;
        if(std::abs(z2)<1.0e-30)z2=0.0;
        return y;
    }
};

Biquad highShelf(double sampleRate,double frequency,double q,double gainDb){
    // De Man implementation used to preserve the BS.1770 48 kHz response
    // while deriving equivalent coefficients at other sample rates.
    const double k=std::tan(kPi*frequency/sampleRate);
    const double vh=std::pow(10.0,gainDb/20.0);
    const double vb=std::pow(vh,0.499666774155);
    const double a0=1.0+k/q+k*k;
    Biquad f;
    f.b0=(vh+vb*k/q+k*k)/a0;
    f.b1=2.0*(k*k-vh)/a0;
    f.b2=(vh-vb*k/q+k*k)/a0;
    f.a1=2.0*(k*k-1.0)/a0;
    f.a2=(1.0-k/q+k*k)/a0;
    return f;
}

Biquad highPass(double sampleRate,double frequency,double q){
    const double k=std::tan(kPi*frequency/sampleRate);
    const double a0=1.0+k/q+k*k;
    Biquad f;
    f.b0=1.0;
    f.b1=-2.0;
    f.b2=1.0;
    f.a1=2.0*(k*k-1.0)/a0;
    f.a2=(1.0-k/q+k*k)/a0;
    // The De Man high-pass numerator is intentionally not divided by a0.
    return f;
}

std::vector<double> kWeightedEnergyFrames(const AudioBuffer& audio){
    if(audio.sampleRate<=0||audio.channels<=0)throw std::invalid_argument("Invalid audio buffer for loudness analysis");
    const auto frames=audio.frames();
    std::vector<double> energy(static_cast<std::size_t>(frames),0.0);
    std::vector<Biquad> shelf(static_cast<std::size_t>(audio.channels));
    std::vector<Biquad> highpass(static_cast<std::size_t>(audio.channels));
    for(int c=0;c<audio.channels;++c){
        // ITU-R BS.1770 K-weighting design parameters.
        shelf[static_cast<std::size_t>(c)]=highShelf(audio.sampleRate,1681.974450955533,0.7071752369554196,3.999843853973347);
        highpass[static_cast<std::size_t>(c)]=highPass(audio.sampleRate,38.13547087602444,0.5003270373238773);
    }
    for(SampleIndex frame=0;frame<frames;++frame){
        const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(audio.channels);
        double sum=0.0;
        for(int c=0;c<audio.channels;++c){
            double x=audio.interleaved[base+static_cast<std::size_t>(c)];
            if(!std::isfinite(x))x=0.0;
            x=shelf[static_cast<std::size_t>(c)].process(x);
            x=highpass[static_cast<std::size_t>(c)].process(x);
            // FLOWDAW currently supports mono/stereo audio. BS.1770 channel
            // weighting is unity for L/R (LFE is not part of this path).
            sum+=x*x;
        }
        energy[static_cast<std::size_t>(frame)]=sum;
    }
    return energy;
}

double loudnessFromEnergy(double e){
    return e>0.0?kLoudnessOffset+10.0*std::log10(e):-std::numeric_limits<double>::infinity();
}

struct EnergyBlock { double energy=0.0; double lufs=-std::numeric_limits<double>::infinity(); };

std::vector<EnergyBlock> blockEnergies(const std::vector<double>& energy,int blockFrames,int stepFrames){
    std::vector<EnergyBlock> out;
    if(blockFrames<=0||stepFrames<=0||energy.size()<static_cast<std::size_t>(blockFrames))return out;
    std::vector<double> prefix(energy.size()+1,0.0);
    for(std::size_t i=0;i<energy.size();++i)prefix[i+1]=prefix[i]+energy[i];
    for(std::size_t start=0;start+static_cast<std::size_t>(blockFrames)<=energy.size();start+=static_cast<std::size_t>(stepFrames)){
        const double e=(prefix[start+static_cast<std::size_t>(blockFrames)]-prefix[start])/static_cast<double>(blockFrames);
        out.push_back({e,loudnessFromEnergy(e)});
    }
    return out;
}

double gatedIntegrated(const std::vector<EnergyBlock>& blocks){
    std::vector<double> absolute;
    for(const auto&b:blocks)if(b.lufs>=kAbsoluteGateLufs)absolute.push_back(b.energy);
    if(absolute.empty())return -std::numeric_limits<double>::infinity();
    const double mean=std::accumulate(absolute.begin(),absolute.end(),0.0)/absolute.size();
    const double relativeGate=loudnessFromEnergy(mean)-10.0;
    double sum=0.0;std::size_t count=0;
    for(const auto&b:blocks)if(b.lufs>=kAbsoluteGateLufs&&b.lufs>=relativeGate){sum+=b.energy;++count;}
    return count?loudnessFromEnergy(sum/static_cast<double>(count)):-std::numeric_limits<double>::infinity();
}

double percentile(std::vector<double> values,double p){
    if(values.empty())return 0.0;
    std::sort(values.begin(),values.end());
    const double pos=std::clamp(p,0.0,1.0)*static_cast<double>(values.size()-1);
    const auto lo=static_cast<std::size_t>(std::floor(pos)),hi=static_cast<std::size_t>(std::ceil(pos));
    const double t=pos-static_cast<double>(lo);
    return values[lo]+(values[hi]-values[lo])*t;
}

double loudnessRange(const std::vector<EnergyBlock>& shortTerm,double integrated){
    if(shortTerm.empty()||!std::isfinite(integrated))return 0.0;
    const double relativeGate=integrated-20.0;
    std::vector<double> kept;
    for(const auto&b:shortTerm)if(b.lufs>=kAbsoluteGateLufs&&b.lufs>=relativeGate)kept.push_back(b.lufs);
    if(kept.size()<2)return 0.0;
    return std::max(0.0,percentile(kept,0.95)-percentile(kept,0.10));
}

double db(double linear){
    return linear>0.0?20.0*std::log10(linear):-std::numeric_limits<double>::infinity();
}

double samplePeak(const AudioBuffer& audio){
    double p=0.0;
    for(float x:audio.interleaved)if(std::isfinite(x))p=std::max(p,std::abs(static_cast<double>(x)));
    return p;
}

double sinc(double x){
    if(std::abs(x)<1.0e-12)return 1.0;
    const double px=kPi*x;
    return std::sin(px)/px;
}

double blackmanWindow(double x,double radius){
    const double ax=std::abs(x);
    if(ax>radius)return 0.0;
    return 0.42+0.5*std::cos(kPi*x/radius)+0.08*std::cos(2.0*kPi*x/radius);
}

double truePeak(const AudioBuffer& audio){
    if(audio.frames()<=0||audio.channels<=0)return 0.0;
    int factor=1;
    if(audio.sampleRate<64000)factor=8;
    else if(audio.sampleRate<128000)factor=4;
    else if(audio.sampleRate<192000)factor=2;
    constexpr int radius=16;
    std::vector<std::vector<double>> kernels(static_cast<std::size_t>(factor));
    for(int phase=0;phase<factor;++phase){
        const double frac=static_cast<double>(phase)/factor;
        auto&kernel=kernels[static_cast<std::size_t>(phase)];
        kernel.resize(radius*2+1);
        double norm=0.0;
        for(int tap=-radius;tap<=radius;++tap){
            const double d=static_cast<double>(tap)-frac;
            const double k=sinc(d)*blackmanWindow(d,radius);
            kernel[static_cast<std::size_t>(tap+radius)]=k;norm+=k;
        }
        if(std::abs(norm)>1.0e-12)for(auto&k:kernel)k/=norm;
    }
    double peak=samplePeak(audio);
    const auto frames=audio.frames();
    for(int c=0;c<audio.channels;++c){
        for(SampleIndex frame=0;frame<frames;++frame){
            for(int phase=1;phase<factor;++phase){
                const auto&kernel=kernels[static_cast<std::size_t>(phase)];
                double y=0.0;
                for(int tap=-radius;tap<=radius;++tap){
                    const auto source=frame+tap;
                    if(source<0||source>=frames)continue;
                    const auto index=static_cast<std::size_t>(source)*static_cast<std::size_t>(audio.channels)+static_cast<std::size_t>(c);
                    double x=audio.interleaved[index];if(!std::isfinite(x))x=0.0;
                    y+=x*kernel[static_cast<std::size_t>(tap+radius)];
                }
                peak=std::max(peak,std::abs(y));
            }
        }
    }
    return peak;
}

} // namespace

MasteringAnalysis analyzeMasteringAudio(const AudioBuffer& audio){
    MasteringAnalysis out;
    if(audio.sampleRate<=0||audio.channels<=0||audio.frames()<=0)return out;
    out.durationSeconds=static_cast<double>(audio.frames())/audio.sampleRate;
    out.samplePeakDbfs=db(samplePeak(audio));
    out.truePeakDbtp=db(truePeak(audio));
    const auto energy=kWeightedEnergyFrames(audio);
    const int momentaryFrames=std::max(1,static_cast<int>(std::lround(0.400*audio.sampleRate)));
    const int stepFrames=std::max(1,static_cast<int>(std::lround(0.100*audio.sampleRate)));
    const auto momentary=blockEnergies(energy,momentaryFrames,stepFrames);
    out.integratedLufs=gatedIntegrated(momentary);
    for(const auto&b:momentary)out.maxMomentaryLufs=std::max(out.maxMomentaryLufs,b.lufs);
    const int shortFrames=std::max(1,static_cast<int>(std::lround(3.0*audio.sampleRate)));
    const auto shortTerm=blockEnergies(energy,shortFrames,stepFrames);
    for(const auto&b:shortTerm)out.maxShortTermLufs=std::max(out.maxShortTermLufs,b.lufs);
    out.loudnessRangeLu=loudnessRange(shortTerm,out.integratedLufs);
    return out;
}

MasteringCompliance evaluateMasteringCompliance(const MasteringAnalysis& analysis,const MasteringTarget& target){
    MasteringCompliance out;
    if(std::isfinite(analysis.integratedLufs)){
        out.loudnessErrorLu=analysis.integratedLufs-target.targetLufs;
        out.loudnessInTolerance=std::abs(out.loudnessErrorLu)<=target.loudnessToleranceLu;
    }
    if(std::isfinite(analysis.truePeakDbtp)){
        out.truePeakMarginDb=target.maxTruePeakDbtp-analysis.truePeakDbtp;
        out.truePeakWithinLimit=out.truePeakMarginDb>=-1.0e-6;
    }
    out.compliant=out.loudnessInTolerance&&out.truePeakWithinLimit;
    return out;
}

double normalizeMasteringGain(AudioBuffer& audio,const MasteringTarget& target,MasteringAnalysis* before,MasteringAnalysis* after){
    const auto initial=analyzeMasteringAudio(audio);
    if(before)*before=initial;
    if(!std::isfinite(initial.integratedLufs)||!std::isfinite(initial.truePeakDbtp)){
        if(after)*after=initial;
        return 0.0;
    }
    const double desired=target.targetLufs-initial.integratedLufs;
    const double peakLimited=target.maxTruePeakDbtp-initial.truePeakDbtp;
    const double gainDb=std::min(desired,peakLimited);
    const double gain=std::pow(10.0,gainDb/20.0);
    for(auto&x:audio.interleaved){
        double y=std::isfinite(x)?static_cast<double>(x):0.0;
        y*=gain;x=static_cast<float>(std::clamp(y,-1.0,1.0));
    }
    if(after)*after=analyzeMasteringAudio(audio);
    return gainDb;
}

} // namespace flowdaw
