#include "flowdaw/Mastering.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}

static AudioBuffer sine(int sr,int channels,double seconds,double frequency,float amplitude){
    const int frames=static_cast<int>(std::lround(seconds*sr));
    AudioBuffer b;b.sampleRate=sr;b.channels=channels;b.interleaved.resize(static_cast<std::size_t>(frames)*channels);
    constexpr double pi=3.14159265358979323846;
    for(int f=0;f<frames;++f){
        const float x=amplitude*static_cast<float>(std::sin(2.0*pi*frequency*f/sr));
        for(int c=0;c<channels;++c)b.interleaved[static_cast<std::size_t>(f)*channels+c]=x;
    }
    return b;
}

static bool anyNonZero(const AudioBuffer& b){
    return std::any_of(b.interleaved.begin(),b.interleaved.end(),[](float x){return x!=0.0f;});
}

int main(){
    try{
        // EBU Tech 3341 minimum-requirement Test 1: stereo 1 kHz sine,
        // -23 dBFS peak per channel, 20 seconds => M/S/I -23.0 ±0.1 LUFS.
        const float ebuAmplitude=static_cast<float>(std::pow(10.0,-23.0/20.0));
        const auto ebuSignal=sine(48000,2,20.0,1000.0,ebuAmplitude);
        const auto ebu=analyzeMasteringAudio(ebuSignal);
        require(std::abs(ebu.integratedLufs+23.0)<=0.10,"EBU Tech 3341 Test 1 integrated loudness");
        require(std::abs(ebu.maxMomentaryLufs+23.0)<=0.10,"EBU Tech 3341 Test 1 momentary loudness");
        require(std::abs(ebu.maxShortTermLufs+23.0)<=0.10,"EBU Tech 3341 Test 1 short-term loudness");

        auto a=sine(48000,2,8.0,1000.0,0.05f);
        auto b=a;for(auto&x:b.interleaved)x*=2.0f;
        const auto ma=analyzeMasteringAudio(a),mb=analyzeMasteringAudio(b);
        require(std::isfinite(ma.integratedLufs)&&std::isfinite(mb.integratedLufs),"integrated loudness must be finite");
        require(std::abs((mb.integratedLufs-ma.integratedLufs)-6.0206)<0.08,"LUFS gain scaling regression");
        require(ma.truePeakDbtp+1.0e-6>=ma.samplePeakDbfs,"true peak must not be below sample peak");
        require(std::isfinite(ma.maxMomentaryLufs)&&std::isfinite(ma.maxShortTermLufs),"EBU meter windows must be finite");
        require(ma.loudnessRangeLu>=0.0,"LRA must be non-negative");

        AudioBuffer gated;gated.sampleRate=48000;gated.channels=2;
        auto silence=sine(48000,2,4.0,1000.0,0.0f);
        auto tone=sine(48000,2,4.0,1000.0,0.05f);
        gated.interleaved=silence.interleaved;gated.interleaved.insert(gated.interleaved.end(),tone.interleaved.begin(),tone.interleaved.end());
        const auto mg=analyzeMasteringAudio(gated);
        require(std::abs(mg.integratedLufs-ma.integratedLufs)<0.8,"R128 gating should reject leading silence");

        MasteringTarget target;target.targetLufs=-23.0;target.maxTruePeakDbtp=-1.0;target.loudnessToleranceLu=0.5;
        auto normalized=a;MasteringAnalysis before,after;
        const double gain=normalizeMasteringGain(normalized,target,&before,&after);
        require(gain>0.0,"quiet programme should receive positive normalization gain");
        require(std::abs(after.integratedLufs-target.targetLufs)<0.20,"normalization must land near requested LUFS");
        require(after.truePeakDbtp<=target.maxTruePeakDbtp+0.05,"normalization must respect true-peak ceiling");
        require(evaluateMasteringCompliance(after,target).compliant,"normalized quiet programme should comply");

        auto hot=sine(48000,2,8.0,1000.0,0.95f);
        MasteringAnalysis hotBefore,hotAfter;
        const double hotGain=normalizeMasteringGain(hot,target,&hotBefore,&hotAfter);
        require(hotAfter.truePeakDbtp<=target.maxTruePeakDbtp+0.05,"hot normalization must obey true-peak ceiling");
        require(hotGain<=target.maxTruePeakDbtp-hotBefore.truePeakDbtp+0.05,"true-peak ceiling must cap requested gain");

        const auto dir=std::filesystem::temp_directory_path()/"flowdaw_phase14";
        std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);
        AudioBuffer zero;zero.sampleRate=48000;zero.channels=2;zero.interleaved.assign(48000*2,0.0f);
        const auto noDither=dir/"silence_nodither16.wav",ditherA=dir/"silence_dither16_a.wav",ditherB=dir/"silence_dither16_b.wav",pcm24=dir/"tone24.wav";
        WavFile::writePcm(noDither,zero,{WavIntegerBitDepth::pcm16,false,123u});
        WavFile::writePcm(ditherA,zero,{WavIntegerBitDepth::pcm16,true,123u});
        WavFile::writePcm(ditherB,zero,{WavIntegerBitDepth::pcm16,true,123u});
        const auto nd=WavFile::read(noDither),da=WavFile::read(ditherA),db=WavFile::read(ditherB);
        require(!anyNonZero(nd),"undithered digital silence must remain zero");
        require(anyNonZero(da),"TPDF dither must decorrelate integer quantization at silence");
        require(da.interleaved==db.interleaved,"TPDF seed must make test/export behavior deterministic");
        WavFile::writePcm(pcm24,a,{WavIntegerBitDepth::pcm24,true,456u});
        const auto reread24=WavFile::read(pcm24);
        require(reread24.frames()==a.frames()&&reread24.sampleRate==a.sampleRate,"PCM24 roundtrip shape");

        Project project;project.sampleRate=48000;project.transport.bpm=120.0;project.name="Phase14 mastering export";
        SampleAsset asset;asset.name="tone";asset.audio=std::make_shared<AudioBuffer>(sine(48000,2,4.0,1000.0,0.05f));const Id sid=asset.id;project.samples.push_back(asset);
        Track track;track.name="Mastering Tone";Clip clip;clip.sampleId=sid;clip.sourceLength=asset.audio->frames();clip.lengthTicks=MusicalTime::samplesToTicks(asset.audio->frames(),120.0,48000);track.clips.push_back(clip);project.tracks.push_back(track);
        MasteringExportOptions opts;opts.encoding=MasteringExportEncoding::pcm24;opts.tpdfDither=true;opts.normalizeToTarget=true;opts.target=target;
        const auto masteredPath=dir/"mastered.wav";
        const auto report=exportMasteringWav(project,masteredPath,opts,0.0);
        require(std::filesystem::is_regular_file(masteredPath),"mastering export file exists");
        require(WavFile::read(masteredPath).frames()>0,"mastering export contains audio");
        require(std::isfinite(report.after.integratedLufs)&&std::isfinite(report.after.truePeakDbtp),"mastering export report is finite");
        require(report.after.truePeakDbtp<=target.maxTruePeakDbtp+0.10,"mastering export respects TP ceiling");

        std::filesystem::remove_all(dir);
        std::cout<<"FLOWDAW Phase 14 mastering tests: PASS\n";
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 14 mastering tests: FAIL: "<<e.what()<<"\n";return 1;
    }
}
