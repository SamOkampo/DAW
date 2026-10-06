#include "flowdaw/Loudness.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}

static AudioBuffer sine(int sr,int channels,double seconds,double frequency,double amplitude){
    AudioBuffer b;b.sampleRate=sr;b.channels=channels;
    const auto frames=static_cast<SampleIndex>(std::llround(seconds*sr));
    b.interleaved.resize(static_cast<std::size_t>(frames)*channels);
    constexpr double pi=3.14159265358979323846;
    for(SampleIndex f=0;f<frames;++f){
        const float x=static_cast<float>(amplitude*std::sin(2.0*pi*frequency*static_cast<double>(f)/sr));
        for(int c=0;c<channels;++c)b.interleaved[static_cast<std::size_t>(f)*channels+c]=x;
    }
    return b;
}

int main(){
    try{
        AudioBuffer silence;silence.sampleRate=48000;silence.channels=2;silence.interleaved.assign(48000*2,0.0f);
        auto sr=MasteringAnalyzer::analyze(silence);
        require(sr.silence&&!std::isfinite(sr.integratedLufs),"silence loudness");

        auto tone=sine(48000,2,6.0,1000.0,0.1);
        auto report=MasteringAnalyzer::analyze(tone);
        require(!report.silence&&std::isfinite(report.integratedLufs),"tone integrated loudness finite");
        require(report.integratedLufs>-21.5&&report.integratedLufs<-18.5,"1 kHz stereo tone loudness outside expected BS.1770-aligned range");
        require(std::abs(report.momentaryMaxLufs-report.integratedLufs)<0.5,"steady tone momentary/integrated mismatch");
        require(std::abs(report.shortTermMaxLufs-report.integratedLufs)<0.5,"steady tone short-term/integrated mismatch");
        require(report.loudnessRangeLu<0.2,"steady tone LRA should be near zero");

        auto quieter=sine(48000,2,6.0,1000.0,0.1*std::pow(10.0,-6.0/20.0));
        auto quietReport=MasteringAnalyzer::analyze(quieter);
        require(std::abs((report.integratedLufs-quietReport.integratedLufs)-6.0)<0.15,"LUFS gain tracking must be ~6 LU");

        AudioBuffer overshoot;overshoot.sampleRate=48000;overshoot.channels=1;
        for(int i=0;i<512;++i){
            const int p=i%4;
            overshoot.interleaved.push_back(p<2?0.95f:-0.95f);
        }
        auto over=MasteringAnalyzer::analyze(overshoot);
        require(over.truePeakLinear>=over.samplePeakLinear,"true peak must never be lower than sample peak");
        require(over.truePeakLinear>0.95,"band-limited oversampling should detect inter-sample excursion");

        auto tone441=sine(44100,2,4.0,1000.0,0.1);
        auto tone96=sine(96000,2,4.0,1000.0,0.1);
        const auto r441=MasteringAnalyzer::analyze(tone441);
        const auto r96=MasteringAnalyzer::analyze(tone96);
        require(std::abs(r441.integratedLufs-r96.integratedLufs)<0.5,"loudness must be sample-rate stable");

        std::cout<<"FLOWDAW Phase 14.1 loudness tests: PASS\n";
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 14.1 loudness tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
