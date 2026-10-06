#include "flowdaw/Mastering.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/Wav.hpp"
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
constexpr double kAbsoluteGate=-70.0;

struct Biquad {
    double b0=1.0,b1=0.0,b2=0.0,a1=0.0,a2=0.0;
    double z1=0.0,z2=0.0;
    double process(double x) noexcept {
        const double y=b0*x+z1;
        z1=b1*x-a1*y+z2;
        z2=b2*x-a2*y;
        return y;
    }
};

Biquad kShelf(int sampleRate){
    // BS.1770 K-weighting pre-filter parameters (De Man implementation).
    constexpr double gainDb=3.999843853973347;
    constexpr double q=0.7071752369554196;
    constexpr double fc=1681.974450955533;
    const double k=std::tan(kPi*fc/static_cast<double>(sampleRate));
    const double vh=std::pow(10.0,gainDb/20.0);
    const double vb=std::pow(vh,0.4996667741545416);
    const double a0=1.0+k/q+k*k;
    Biquad f;
    f.b0=(vh+vb*k/q+k*k)/a0;
    f.b1=2.0*(k*k-vh)/a0;
    f.b2=(vh-vb*k/q+k*k)/a0;
    f.a1=2.0*(k*k-1.0)/a0;
    f.a2=(1.0-k/q+k*k)/a0;
    return f;
}

Biquad kHighPass(int sampleRate){
    constexpr double q=0.5003270373238773;
    constexpr double fc=38.13547087602444;
    const double k=std::tan(kPi*fc/static_cast<double>(sampleRate));
    const double a0=1.0+k/q+k*k;
    Biquad f;
    f.b0=1.0/a0;
    f.b1=-2.0/a0;
    f.b2=1.0/a0;
    f.a1=2.0*(k*k-1.0)/a0;
    f.a2=(1.0-k/q+k*k)/a0;
    return f;
}

double loudnessFromEnergy(double energy){
    if(!(energy>0.0)||!std::isfinite(energy))return -std::numeric_limits<double>::infinity();
    return kLoudnessOffset+10.0*std::log10(energy);
}

double percentile(std::vector<double> values,double p){
    if(values.empty())return 0.0;
    std::sort(values.begin(),values.end());
    const double index=(values.size()-1)*std::clamp(p,0.0,1.0);
    const auto lo=static_cast<std::size_t>(std::floor(index));
    const auto hi=static_cast<std::size_t>(std::ceil(index));
    if(lo==hi)return values[lo];
    const double frac=index-static_cast<double>(lo);
    return values[lo]+(values[hi]-values[lo])*frac;
}

std::vector<double> windowEnergies(
    const std::vector<double>& prefix,
    int sampleRate,
    double windowSeconds,
    double stepSeconds){
    const auto totalFrames=prefix.size()>0?prefix.size()-1:0;
    const std::size_t window=std::max<std::size_t>(1,static_cast<std::size_t>(std::llround(windowSeconds*sampleRate)));
    const std::size_t step=std::max<std::size_t>(1,static_cast<std::size_t>(std::llround(stepSeconds*sampleRate)));
    std::vector<double> out;
    if(totalFrames<window)return out;
    out.reserve(1+(totalFrames-window)/step);
    for(std::size_t start=0;start+window<=totalFrames;start+=step){
        const double sum=prefix[start+window]-prefix[start];
        out.push_back(sum/static_cast<double>(window));
    }
    return out;
}

double gatedIntegrated(const std::vector<double>& blockEnergy){
    std::vector<double> absolute;
    absolute.reserve(blockEnergy.size());
    for(const double e:blockEnergy)if(loudnessFromEnergy(e)>=kAbsoluteGate)absolute.push_back(e);
    if(absolute.empty())return -std::numeric_limits<double>::infinity();
    const double mean=std::accumulate(absolute.begin(),absolute.end(),0.0)/absolute.size();
    const double relativeThreshold=loudnessFromEnergy(mean)-10.0;
    double sum=0.0;std::size_t count=0;
    for(const double e:absolute){
        if(loudnessFromEnergy(e)>=relativeThreshold){sum+=e;++count;}
    }
    return count?loudnessFromEnergy(sum/static_cast<double>(count)):-std::numeric_limits<double>::infinity();
}

double sinc(double x){
    if(std::abs(x)<1.0e-12)return 1.0;
    const double pix=kPi*x;
    return std::sin(pix)/pix;
}

double blackman(double x){
    // x normalized to [-1,1].
    const double a=std::clamp((x+1.0)*0.5,0.0,1.0);
    return 0.42-0.5*std::cos(2.0*kPi*a)+0.08*std::cos(4.0*kPi*a);
}

double truePeakLinear(const AudioBuffer& audio){
    if(audio.channels<=0||audio.frames()<=0)return 0.0;
    double peak=0.0;
    for(float x:audio.interleaved)peak=std::max(peak,std::abs(static_cast<double>(x)));

    // Offline 4x band-limited interpolation. 24-tap Blackman-windowed sinc
    // gives deterministic high-quality inter-sample peak estimation without
    // changing the realtime callback.
    constexpr int radius=12;
    for(int channel=0;channel<audio.channels;++channel){
        for(SampleIndex n=0;n+1<audio.frames();++n){
            for(int phase=1;phase<4;++phase){
                const double t=static_cast<double>(n)+0.25*phase;
                double y=0.0,weight=0.0;
                for(int k=-radius+1;k<=radius;++k){
                    const auto index=static_cast<SampleIndex>(std::floor(t))+k;
                    if(index<0||index>=audio.frames())continue;
                    const double d=t-static_cast<double>(index);
                    if(std::abs(d)>radius)continue;
                    const double w=blackman(d/static_cast<double>(radius));
                    const double coefficient=sinc(d)*w;
                    y+=static_cast<double>(audio.interleaved[static_cast<std::size_t>(index)*audio.channels+channel])*coefficient;
                    weight+=coefficient;
                }
                if(std::abs(weight)>1.0e-12)y/=weight;
                peak=std::max(peak,std::abs(y));
            }
        }
    }
    return peak;
}

double db(double linear){
    return linear>0.0?20.0*std::log10(linear):-std::numeric_limits<double>::infinity();
}

void applyGain(AudioBuffer& audio,double gainDb){
    const double gain=std::pow(10.0,gainDb/20.0);
    for(float& x:audio.interleaved){
        const double y=static_cast<double>(x)*gain;
        x=static_cast<float>(std::clamp(y,-1.0,1.0));
    }
}

} // namespace

MasteringMetrics analyzeMastering(const AudioBuffer& audio){
    if(audio.sampleRate<8000||audio.channels<1||audio.channels>2)throw std::invalid_argument("Mastering analysis supports mono/stereo PCM at a valid sample rate");
    MasteringMetrics out;
    const auto frames=audio.frames();
    if(frames<=0)return out;

    std::vector<double> weightedEnergy(static_cast<std::size_t>(frames),0.0);
    std::vector<Biquad> shelves(static_cast<std::size_t>(audio.channels));
    std::vector<Biquad> highpasses(static_cast<std::size_t>(audio.channels));
    for(int channel=0;channel<audio.channels;++channel){shelves[channel]=kShelf(audio.sampleRate);highpasses[channel]=kHighPass(audio.sampleRate);}

    double samplePeak=0.0;
    for(SampleIndex frame=0;frame<frames;++frame){
        double sum=0.0;
        for(int channel=0;channel<audio.channels;++channel){
            const float raw=audio.interleaved[static_cast<std::size_t>(frame)*audio.channels+channel];
            samplePeak=std::max(samplePeak,std::abs(static_cast<double>(raw)));
            const double y=highpasses[static_cast<std::size_t>(channel)].process(shelves[static_cast<std::size_t>(channel)].process(raw));
            sum+=y*y;
        }
        weightedEnergy[static_cast<std::size_t>(frame)]=sum;
    }

    std::vector<double> prefix(weightedEnergy.size()+1,0.0);
    for(std::size_t i=0;i<weightedEnergy.size();++i)prefix[i+1]=prefix[i]+weightedEnergy[i];

    const auto momentary=windowEnergies(prefix,audio.sampleRate,0.400,0.100);
    const auto shortTerm=windowEnergies(prefix,audio.sampleRate,3.000,1.000);
    out.integratedLufs=gatedIntegrated(momentary);
    for(const double e:momentary)out.momentaryMaxLufs=std::max(out.momentaryMaxLufs,loudnessFromEnergy(e));
    for(const double e:shortTerm)out.shortTermMaxLufs=std::max(out.shortTermMaxLufs,loudnessFromEnergy(e));

    if(!shortTerm.empty()){
        std::vector<double> absolute;
        for(const double e:shortTerm)if(loudnessFromEnergy(e)>=kAbsoluteGate)absolute.push_back(e);
        if(!absolute.empty()){
            const double mean=std::accumulate(absolute.begin(),absolute.end(),0.0)/absolute.size();
            const double relative=loudnessFromEnergy(mean)-20.0;
            std::vector<double> gated;
            for(const double e:absolute)if(loudnessFromEnergy(e)>=relative)gated.push_back(loudnessFromEnergy(e));
            if(gated.size()>=2)out.loudnessRangeLu=std::max(0.0,percentile(gated,0.95)-percentile(gated,0.10));
        }
    }

    out.maxSamplePeakDbFS=db(samplePeak);
    out.maxTruePeakDbTP=db(truePeakLinear(audio));
    return out;
}

LoudnessNormalizationResult normalizeLoudness(AudioBuffer& audio,const LoudnessTarget& target){
    LoudnessNormalizationResult result;
    result.before=analyzeMastering(audio);
    if(!std::isfinite(result.before.integratedLufs)){
        result.after=result.before;
        return result;
    }
    result.requestedGainDb=target.targetLufs-result.before.integratedLufs;
    const double peakLimitedGain=std::isfinite(result.before.maxTruePeakDbTP)
        ?target.maxTruePeakDbTP-result.before.maxTruePeakDbTP
        :result.requestedGainDb;
    result.appliedGainDb=std::min(result.requestedGainDb,peakLimitedGain);
    result.limitedByTruePeak=result.appliedGainDb+1.0e-9<result.requestedGainDb;
    applyGain(audio,result.appliedGainDb);
    result.after=analyzeMastering(audio);
    result.targetWithinTolerance=std::isfinite(result.after.integratedLufs)
        &&std::abs(result.after.integratedLufs-target.targetLufs)<=target.toleranceLu
        &&result.after.maxTruePeakDbTP<=target.maxTruePeakDbTP+0.05;
    return result;
}

MasterExportReport exportMasteredWav(
    const Project& project,
    const std::filesystem::path& path,
    const MasterExportOptions& options,
    double tailSeconds,
    std::shared_ptr<PluginHost> pluginHost){
    auto audio=renderProjectOffline(project,tailSeconds,std::move(pluginHost));
    MasterExportReport report;
    report.rendered=analyzeMastering(audio);
    report.bitDepth=options.bitDepth;
    report.dither=options.dither;
    if(options.normalizeLoudness){
        const auto normalized=normalizeLoudness(audio,options.target);
        report.appliedGainDb=normalized.appliedGainDb;
        report.normalized=true;
        report.limitedByTruePeak=normalized.limitedByTruePeak;
        report.targetWithinTolerance=normalized.targetWithinTolerance;
    }
    report.delivered=analyzeMastering(audio);
    std::filesystem::create_directories(path.parent_path().empty()?std::filesystem::path("."):path.parent_path());
    switch(options.bitDepth){
        case MasterBitDepth::Float32: WavFile::writeFloat32(path,audio);break;
        case MasterBitDepth::PCM24: WavFile::writePcm24(path,audio,options.dither==DitherMode::TPDF);break;
        case MasterBitDepth::PCM16: WavFile::writePcm16(path,audio,options.dither==DitherMode::TPDF);break;
    }
    return report;
}

} // namespace flowdaw
