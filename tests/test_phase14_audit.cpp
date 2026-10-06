#include "flowdaw/Mastering.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace flowdaw;

static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}

static AudioBuffer sine(int sr,double seconds,double frequency,double amplitude){
    AudioBuffer b;b.sampleRate=sr;b.channels=2;
    const auto frames=static_cast<SampleIndex>(std::llround(seconds*sr));
    b.interleaved.resize(static_cast<std::size_t>(frames)*2);
    constexpr double pi=3.14159265358979323846;
    for(SampleIndex f=0;f<frames;++f){
        const float x=static_cast<float>(amplitude*std::sin(2.0*pi*frequency*f/sr));
        b.interleaved[static_cast<std::size_t>(f)*2]=x;b.interleaved[static_cast<std::size_t>(f)*2+1]=x;
    }
    return b;
}

static bool sameAudio(const AudioBuffer&a,const AudioBuffer&b){
    return a.sampleRate==b.sampleRate&&a.channels==b.channels&&a.interleaved==b.interleaved;
}

int main(){
    try{
        for(const int sr:{44100,48000,96000}){
            auto tone=sine(sr,4.0,1000.0,0.1);
            const auto m=analyzeMastering(tone);
            require(std::isfinite(m.integratedLufs),"multi-rate integrated loudness must be finite");
            require(std::abs(m.integratedLufs-(-20.04))<0.28,"multi-rate K-weighted calibration drift");
            require(m.maxTruePeakDbTP< -19.5&&m.maxTruePeakDbTP> -20.5,"multi-rate true-peak tone calibration drift");
        }

        auto finite=sine(48000,4.0,1000.0,0.05);
        finite.interleaved[100]=std::numeric_limits<float>::quiet_NaN();
        finite.interleaved[101]=std::numeric_limits<float>::infinity();
        const auto sanitized=analyzeMastering(finite);
        require(std::isfinite(sanitized.integratedLufs),"non-finite source sample poisoned loudness analysis");
        require(std::isfinite(sanitized.maxTruePeakDbTP),"non-finite source sample poisoned true-peak analysis");

        // Deterministic TPDF makes test/audit artefacts reproducible.
        const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase14_audit";
        std::filesystem::remove_all(root);std::filesystem::create_directories(root);
        AudioBuffer silence;silence.sampleRate=48000;silence.channels=2;silence.interleaved.assign(8192,0.0f);
        WavFile::writePcm16(root/"a16.wav",silence,true);WavFile::writePcm16(root/"b16.wav",silence,true);
        WavFile::writePcm24(root/"a24.wav",silence,true);WavFile::writePcm24(root/"b24.wav",silence,true);
        require(sameAudio(WavFile::read(root/"a16.wav"),WavFile::read(root/"b16.wav")),"PCM16 TPDF must be deterministic for regression tests");
        require(sameAudio(WavFile::read(root/"a24.wav"),WavFile::read(root/"b24.wav")),"PCM24 TPDF must be deterministic for regression tests");

        // Float32 delivery is intentionally undithered and must roundtrip closely.
        auto source=sine(48000,1.0,997.0,0.12345);
        WavFile::writeFloat32(root/"float.wav",source);
        const auto floatBack=WavFile::read(root/"float.wav");
        require(floatBack.interleaved==source.interleaved,"Float32 WAV export must not add dither or quantization");

        std::filesystem::remove_all(root);
        std::cout<<"FLOWDAW Phase 14.6 mastering regression audit tests: PASS\n";return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 14.6 mastering regression audit tests: FAIL: "<<e.what()<<"\n";return 1;
    }
}
