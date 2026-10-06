#include "flowdaw/Mastering.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>

namespace flowdaw {
namespace {
constexpr double kPi=3.14159265358979323846;
constexpr double kFloorDb=-120.0;

struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    double process(double x) noexcept {
        const double y=b0*x+z1;
        z1=b1*x-a1*y+z2;
        z2=b2*x-a2*y;
        return y;
    }
};

Biquad highPass(int sr,double f,double q){
    const double w=2.0*kPi*f/sr, c=std::cos(w), s=std::sin(w), alpha=s/(2.0*q);
    const double a0=1.0+alpha;
    Biquad b; b.b0=((1.0+c)/2.0)/a0; b.b1=(-(1.0+c))/a0; b.b2=b.b0;
    b.a1=(-2.0*c)/a0; b.a2=(1.0-alpha)/a0; return b;
}
Biquad highShelf(int sr,double f,double gainDb,double slope){
    const double A=std::pow(10.0,gainDb/40.0), w=2.0*kPi*f/sr, c=std::cos(w), s=std::sin(w);
    const double alpha=s/2.0*std::sqrt((A+1.0/A)*(1.0/slope-1.0)+2.0);
    const double beta=2.0*std::sqrt(A)*alpha;
    const double a0=(A+1.0)-(A-1.0)*c+beta;
    Biquad b;
    b.b0=A*((A+1.0)+(A-1.0)*c+beta)/a0;
    b.b1=-2.0*A*((A-1.0)+(A+1.0)*c)/a0;
    b.b2=A*((A+1.0)+(A-1.0)*c-beta)/a0;
    b.a1=2.0*((A-1.0)-(A+1.0)*c)/a0;
    b.a2=((A+1.0)-(A-1.0)*c-beta)/a0;
    return b;
}
double blockLufs(const std::vector<double>&weighted,std::size_t start,std::size_t length,int channels){
    if(!length||channels<=0)return kFloorDb;
    long double sum=0.0;
    const std::size_t end=std::min(weighted.size(),start+length*static_cast<std::size_t>(channels));
    for(std::size_t i=start;i<end;++i)sum+=weighted[i]*weighted[i];
    const auto frames=(end-start)/static_cast<std::size_t>(channels);
    if(!frames)return kFloorDb;
    const double mean=static_cast<double>(sum/(static_cast<long double>(frames)*channels));
    if(mean<=1e-15)return kFloorDb;
    return -0.691+10.0*std::log10(mean);
}
double percentile(std::vector<double> values,double p){
    if(values.empty())return kFloorDb;
    std::sort(values.begin(),values.end());
    const double pos=std::clamp(p,0.0,1.0)*(values.size()-1);
    const auto lo=static_cast<std::size_t>(std::floor(pos)), hi=static_cast<std::size_t>(std::ceil(pos));
    if(lo==hi)return values[lo];
    const double t=pos-lo;return values[lo]*(1.0-t)+values[hi]*t;
}
double sinc(double x){if(std::abs(x)<1e-12)return 1.0;return std::sin(kPi*x)/(kPi*x);}
double truePeak4x(const AudioBuffer&a){
    if(a.channels<=0||a.frames()<=0)return 0.0;
    double peak=0.0;
    constexpr int radius=8;
    for(int ch=0;ch<a.channels;++ch){
        for(SampleIndex n=0;n<a.frames();++n){
            for(int phase=0;phase<4;++phase){
                const double t=phase/4.0; long double y=0.0, norm=0.0;
                for(int k=-radius;k<=radius;++k){
                    const auto idx=n+k;
                    if(idx<0||idx>=a.frames())continue;
                    const double x=static_cast<double>(k)-t;
                    const double win=0.5+0.5*std::cos(kPi*x/(radius+1.0));
                    const double w=sinc(x)*win;
                    y+=static_cast<long double>(a.interleaved[static_cast<std::size_t>(idx)*a.channels+ch])*w;
                    norm+=w;
                }
                if(std::abs(static_cast<double>(norm))>1e-12)y/=norm;
                peak=std::max(peak,std::abs(static_cast<double>(y)));
            }
        }
    }
    return peak;
}
}

double linearToDb(double value) noexcept {return value>1e-12?20.0*std::log10(value):kFloorDb;}

LoudnessReport analyzeLoudness(const AudioBuffer&a){
    LoudnessReport r;if(a.channels<=0||a.sampleRate<=0||a.frames()<=0)return r;
    std::vector<double>w(a.interleaved.size());
    std::vector<Biquad>shelf(static_cast<std::size_t>(a.channels)),hp(static_cast<std::size_t>(a.channels));
    for(int ch=0;ch<a.channels;++ch){shelf[ch]=highShelf(a.sampleRate,1681.974450955533,3.99984385397,1.0);hp[ch]=highPass(a.sampleRate,38.13547087602444,0.5003270373238773);}
    double samplePeak=0.0;
    for(SampleIndex f=0;f<a.frames();++f)for(int ch=0;ch<a.channels;++ch){
        const auto i=static_cast<std::size_t>(f)*a.channels+ch;
        const double x=a.interleaved[i];samplePeak=std::max(samplePeak,std::abs(x));
        w[i]=hp[ch].process(shelf[ch].process(x));
    }
    r.samplePeakDbfs=linearToDb(samplePeak);r.truePeakDbtp=linearToDb(truePeak4x(a));
    const std::size_t step=static_cast<std::size_t>(std::max(1,a.sampleRate/10));
    const std::size_t mFrames=static_cast<std::size_t>(std::max(1,a.sampleRate*4/10));
    const std::size_t sFrames=static_cast<std::size_t>(std::max(1,a.sampleRate*3));
    std::vector<double>blocks400,blocks3s;
    for(std::size_t startFrame=0;startFrame+mFrames<=static_cast<std::size_t>(a.frames());startFrame+=step){
        const double l=blockLufs(w,startFrame*a.channels,mFrames,a.channels);blocks400.push_back(l);r.momentaryMaxLufs=std::max(r.momentaryMaxLufs,l);
    }
    for(std::size_t startFrame=0;startFrame+sFrames<=static_cast<std::size_t>(a.frames());startFrame+=step){
        const double l=blockLufs(w,startFrame*a.channels,sFrames,a.channels);blocks3s.push_back(l);r.shortTermMaxLufs=std::max(r.shortTermMaxLufs,l);
    }
    std::vector<double>absGated;for(double l:blocks400)if(l>=-70.0)absGated.push_back(l);
    if(!absGated.empty()){
        long double power=0.0;for(double l:absGated)power+=std::pow(10.0,(l+0.691)/10.0);power/=absGated.size();
        const double ungated=-0.691+10.0*std::log10(static_cast<double>(power));
        const double rel=ungated-10.0;
        std::vector<double>relGated;for(double l:absGated)if(l>=rel)relGated.push_back(l);
        if(!relGated.empty()){power=0.0;for(double l:relGated)power+=std::pow(10.0,(l+0.691)/10.0);power/=relGated.size();r.integratedLufs=-0.691+10.0*std::log10(static_cast<double>(power));}
    }
    std::vector<double>lra;for(double l:blocks3s)if(l>=-70.0&&l>=r.integratedLufs-20.0)lra.push_back(l);
    if(lra.size()>=2)r.loudnessRangeLu=std::max(0.0,percentile(lra,0.95)-percentile(lra,0.10));
    return r;
}

void applyMasteringTarget(AudioBuffer&a,const MasteringTarget&t){
    if(a.interleaved.empty())return;
    auto report=analyzeLoudness(a);
    if(report.integratedLufs<=-119.0)return;
    double gainDb=t.integratedLufs-report.integratedLufs;
    const double predictedTp=report.truePeakDbtp+gainDb;
    if(predictedTp>t.truePeakCeilingDbtp)gainDb-=predictedTp-t.truePeakCeilingDbtp;
    const double gain=std::pow(10.0,gainDb/20.0);
    for(auto&x:a.interleaved)x=static_cast<float>(std::clamp(static_cast<double>(x)*gain,-1.0,1.0));
}

} // namespace flowdaw
