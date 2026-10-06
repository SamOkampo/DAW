#include "flowdaw/Loudness.hpp"
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
    double b0=1.0,b1=0.0,b2=0.0,a1=0.0,a2=0.0,z1=0.0,z2=0.0;
    double process(double x) noexcept {
        const double y=b0*x+z1;
        z1=b1*x-a1*y+z2;
        z2=b2*x-a2*y;
        return y;
    }
};

Biquad highPass(double sampleRate,double frequency,double q){
    const double w0=2.0*kPi*frequency/sampleRate;
    const double cw=std::cos(w0), sw=std::sin(w0), alpha=sw/(2.0*q);
    const double a0=1.0+alpha;
    Biquad f;
    f.b0=((1.0+cw)*0.5)/a0;
    f.b1=(-(1.0+cw))/a0;
    f.b2=((1.0+cw)*0.5)/a0;
    f.a1=(-2.0*cw)/a0;
    f.a2=(1.0-alpha)/a0;
    return f;
}

Biquad highShelf(double sampleRate,double frequency,double gainDb,double slope){
    const double A=std::pow(10.0,gainDb/40.0);
    const double w0=2.0*kPi*frequency/sampleRate;
    const double cw=std::cos(w0), sw=std::sin(w0);
    const double S=std::max(0.05,slope);
    const double alpha=sw*0.5*std::sqrt((A+1.0/A)*(1.0/S-1.0)+2.0);
    const double twoSqrtAAlpha=2.0*std::sqrt(A)*alpha;
    const double a0=(A+1.0)-(A-1.0)*cw+twoSqrtAAlpha;
    Biquad f;
    f.b0=A*((A+1.0)+(A-1.0)*cw+twoSqrtAAlpha)/a0;
    f.b1=-2.0*A*((A-1.0)+(A+1.0)*cw)/a0;
    f.b2=A*((A+1.0)+(A-1.0)*cw-twoSqrtAAlpha)/a0;
    f.a1=2.0*((A-1.0)-(A+1.0)*cw)/a0;
    f.a2=((A+1.0)-(A-1.0)*cw-twoSqrtAAlpha)/a0;
    return f;
}

double energyToLufs(double energy){
    return energy>1.0e-20?kLoudnessOffset+10.0*std::log10(energy):-std::numeric_limits<double>::infinity();
}

double mean(const std::vector<double>& values){
    if(values.empty())return 0.0;
    return std::accumulate(values.begin(),values.end(),0.0)/static_cast<double>(values.size());
}

double percentile(std::vector<double> values,double p){
    if(values.empty())return -std::numeric_limits<double>::infinity();
    std::sort(values.begin(),values.end());
    const double position=std::clamp(p,0.0,1.0)*static_cast<double>(values.size()-1);
    const auto lo=static_cast<std::size_t>(std::floor(position));
    const auto hi=static_cast<std::size_t>(std::ceil(position));
    const double frac=position-static_cast<double>(lo);
    return values[lo]+(values[hi]-values[lo])*frac;
}

std::vector<double> blockEnergies(const std::vector<double>& frameEnergy,int sampleRate,double windowSeconds,double stepSeconds){
    const std::size_t window=static_cast<std::size_t>(std::max(1.0,std::round(windowSeconds*sampleRate)));
    const std::size_t step=static_cast<std::size_t>(std::max(1.0,std::round(stepSeconds*sampleRate)));
    std::vector<double> blocks;
    if(frameEnergy.empty())return blocks;
    if(frameEnergy.size()<window){
        blocks.push_back(std::accumulate(frameEnergy.begin(),frameEnergy.end(),0.0)/static_cast<double>(frameEnergy.size()));
        return blocks;
    }
    double sum=std::accumulate(frameEnergy.begin(),frameEnergy.begin()+static_cast<std::ptrdiff_t>(window),0.0);
    for(std::size_t start=0;;start+=step){
        if(start>0){
            const std::size_t previous=start-step;
            const std::size_t removeEnd=std::min(previous+step,frameEnergy.size());
            for(std::size_t i=previous;i<removeEnd;++i)sum-=frameEnergy[i];
            const std::size_t addStart=previous+window;
            const std::size_t addEnd=std::min(start+window,frameEnergy.size());
            for(std::size_t i=addStart;i<addEnd;++i)sum+=frameEnergy[i];
        }
        if(start+window>frameEnergy.size())break;
        blocks.push_back(sum/static_cast<double>(window));
        if(start+step+window>frameEnergy.size())break;
    }
    return blocks;
}

double gatedIntegrated(const std::vector<double>& blocks){
    std::vector<double> absolute;
    absolute.reserve(blocks.size());
    for(double e:blocks)if(energyToLufs(e)>=kAbsoluteGate)absolute.push_back(e);
    if(absolute.empty())return -std::numeric_limits<double>::infinity();
    const double ungated=energyToLufs(mean(absolute));
    const double relative=ungated-10.0;
    std::vector<double> gated;
    gated.reserve(absolute.size());
    for(double e:absolute)if(energyToLufs(e)>=std::max(kAbsoluteGate,relative))gated.push_back(e);
    return gated.empty()?-std::numeric_limits<double>::infinity():energyToLufs(mean(gated));
}

double sinc(double x){
    if(std::abs(x)<1.0e-12)return 1.0;
    return std::sin(kPi*x)/(kPi*x);
}

double hann(double x,double radius){
    const double a=std::abs(x);
    if(a>=radius)return 0.0;
    return 0.5*(1.0+std::cos(kPi*a/radius));
}

double interpolatedSample(const AudioBuffer&audio,SampleIndex frame,int channel,double phase){
    constexpr int radius=16;
    double sum=0.0,norm=0.0;
    const auto frames=audio.frames();
    const double position=static_cast<double>(frame)+phase;
    const int center=static_cast<int>(std::floor(position));
    for(int tap=center-radius+1;tap<=center+radius;++tap){
        const double distance=position-static_cast<double>(tap);
        const double weight=sinc(distance)*hann(distance,static_cast<double>(radius));
        if(tap>=0&&tap<frames){
            sum+=static_cast<double>(audio.interleaved[static_cast<std::size_t>(tap)*audio.channels+static_cast<std::size_t>(channel)])*weight;
        }
        norm+=weight;
    }
    return std::abs(norm)>1.0e-12?sum/norm:0.0;
}

} // namespace

double MasteringAnalyzer::truePeakLinear(const AudioBuffer&audio){
    if(audio.channels<1||audio.channels>2||audio.sampleRate<=0)return 0.0;
    const auto frames=audio.frames();
    if(frames<=0)return 0.0;
    double peak=0.0;
    for(int channel=0;channel<audio.channels;++channel){
        for(SampleIndex frame=0;frame<frames;++frame){
            peak=std::max(peak,std::abs(static_cast<double>(audio.interleaved[static_cast<std::size_t>(frame)*audio.channels+static_cast<std::size_t>(channel)])));
            if(frame+1<frames){
                peak=std::max(peak,std::abs(interpolatedSample(audio,frame,channel,0.25)));
                peak=std::max(peak,std::abs(interpolatedSample(audio,frame,channel,0.50)));
                peak=std::max(peak,std::abs(interpolatedSample(audio,frame,channel,0.75)));
            }
        }
    }
    return peak;
}

LoudnessReport MasteringAnalyzer::analyze(const AudioBuffer&audio){
    if(audio.channels<1||audio.channels>2||audio.sampleRate<=0)throw std::invalid_argument("MasteringAnalyzer requires mono/stereo audio with a valid sample rate");
    LoudnessReport report;
    const auto frames=audio.frames();
    if(frames<=0)return report;

    std::vector<Biquad> shelf(static_cast<std::size_t>(audio.channels));
    std::vector<Biquad> highpass(static_cast<std::size_t>(audio.channels));
    for(int channel=0;channel<audio.channels;++channel){
        // BS.1770 K-weighting targets: +4 dB high shelf around 1.68 kHz and
        // RLB high-pass around 38 Hz. These cookbook forms are evaluated at
        // the actual project sample rate.
        shelf[static_cast<std::size_t>(channel)]=highShelf(audio.sampleRate,1681.974450955533,3.999843853973347,1.0);
        highpass[static_cast<std::size_t>(channel)]=highPass(audio.sampleRate,38.13547087602444,0.5003270373238773);
    }

    std::vector<double> energy(static_cast<std::size_t>(frames),0.0);
    double samplePeak=0.0;
    for(SampleIndex frame=0;frame<frames;++frame){
        double e=0.0;
        for(int channel=0;channel<audio.channels;++channel){
            const auto index=static_cast<std::size_t>(frame)*audio.channels+static_cast<std::size_t>(channel);
            const double raw=std::isfinite(audio.interleaved[index])?static_cast<double>(audio.interleaved[index]):0.0;
            samplePeak=std::max(samplePeak,std::abs(raw));
            const double weighted=highpass[static_cast<std::size_t>(channel)].process(shelf[static_cast<std::size_t>(channel)].process(raw));
            e+=weighted*weighted;
        }
        energy[static_cast<std::size_t>(frame)]=e;
    }

    const auto momentary=blockEnergies(energy,audio.sampleRate,0.400,0.100);
    const auto shortTerm=blockEnergies(energy,audio.sampleRate,3.000,1.000);
    report.integratedLufs=gatedIntegrated(momentary);
    for(double e:momentary)report.momentaryMaxLufs=std::max(report.momentaryMaxLufs,energyToLufs(e));
    for(double e:shortTerm)report.shortTermMaxLufs=std::max(report.shortTermMaxLufs,energyToLufs(e));

    std::vector<double> lraCandidates;
    const double lraRelative=std::isfinite(report.integratedLufs)?report.integratedLufs-20.0:kAbsoluteGate;
    for(double e:shortTerm){
        const double l=energyToLufs(e);
        if(l>=kAbsoluteGate&&l>=lraRelative)lraCandidates.push_back(l);
    }
    if(lraCandidates.size()>=2)report.loudnessRangeLu=std::max(0.0,percentile(lraCandidates,0.95)-percentile(lraCandidates,0.10));

    report.samplePeakLinear=samplePeak;
    report.samplePeakDbfs=samplePeak>0.0?20.0*std::log10(samplePeak):-std::numeric_limits<double>::infinity();
    report.truePeakLinear=truePeakLinear(audio);
    report.truePeakDbtp=report.truePeakLinear>0.0?20.0*std::log10(report.truePeakLinear):-std::numeric_limits<double>::infinity();
    report.silence=!std::isfinite(report.integratedLufs);
    return report;
}

} // namespace flowdaw
