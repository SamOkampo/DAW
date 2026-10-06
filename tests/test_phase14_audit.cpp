#include "flowdaw/Loudness.hpp"
#include "flowdaw/RealtimeMeter.hpp"
#include "flowdaw/Wav.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static AudioBuffer sine(int sr,double seconds,double hz,double amp){
    AudioBuffer b;b.sampleRate=sr;b.channels=2;const auto frames=static_cast<SampleIndex>(std::llround(seconds*sr));b.interleaved.resize(static_cast<std::size_t>(frames)*2);
    constexpr double pi=3.14159265358979323846;
    for(SampleIndex i=0;i<frames;++i){float x=static_cast<float>(amp*std::sin(2*pi*hz*i/sr));b.interleaved[static_cast<std::size_t>(i)*2]=x;b.interleaved[static_cast<std::size_t>(i)*2+1]=x;}
    return b;
}
int main(){
 try{
    const auto a44=MasteringAnalyzer::analyze(sine(44100,4.0,1000.0,0.08));
    const auto a48=MasteringAnalyzer::analyze(sine(48000,4.0,1000.0,0.08));
    const auto a96=MasteringAnalyzer::analyze(sine(96000,4.0,1000.0,0.08));
    require(std::abs(a44.integratedLufs-a48.integratedLufs)<0.6&&std::abs(a96.integratedLufs-a48.integratedLufs)<0.6,"loudness sample-rate drift");

    auto pathological=sine(48000,1.0,997.0,0.1);pathological.interleaved[5]=std::numeric_limits<float>::quiet_NaN();pathological.interleaved[7]=std::numeric_limits<float>::infinity();
    const auto pathReport=MasteringAnalyzer::analyze(pathological);
    require(std::isfinite(pathReport.integratedLufs)&&std::isfinite(pathReport.truePeakDbtp),"non-finite input escaped mastering analysis");

    RealtimeMeterState live;const float block[]={-1.0f,0.5f,0.5f,-0.25f};live.process(block,2,2);const auto liveReading=live.snapshot();
    require(liveReading.samplePeakLeft>=0.99f&&liveReading.rmsLeft>0.0f,"legacy realtime meter regression");

    const auto dir=std::filesystem::temp_directory_path()/"flowdaw_phase14_audit";std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);
    AudioBuffer silence;silence.sampleRate=48000;silence.channels=2;silence.interleaved.assign(10000,0.0f);
    WavFile::write(dir/"a.wav",silence,{WavEncoding::PCM24,DitherMode::TPDF,42});
    WavFile::write(dir/"b.wav",silence,{WavEncoding::PCM24,DitherMode::TPDF,42});
    WavFile::write(dir/"c.wav",silence,{WavEncoding::PCM24,DitherMode::TPDF,43});
    const auto a=WavFile::read(dir/"a.wav"),b=WavFile::read(dir/"b.wav"),c=WavFile::read(dir/"c.wav");
    require(a.interleaved==b.interleaved,"TPDF must be deterministic for the same explicit seed");
    require(a.interleaved!=c.interleaved,"different TPDF seeds must decorrelate the quantization noise");
    double mean=0.0;for(float x:a.interleaved)mean+=x;mean/=a.interleaved.size();
    require(std::abs(mean)<2.0/8388608.0,"TPDF silence mean is biased");
    std::filesystem::remove_all(dir);
    std::cout<<"FLOWDAW Phase 14.4 mastering audit tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FLOWDAW Phase 14.4 mastering audit tests: FAIL: "<<e.what()<<"\n";return 1;}
}
