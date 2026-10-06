#include "flowdaw/Mastering.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}

static AudioBuffer sine(int sampleRate,int channels,double seconds,double frequency,double amplitude,double phase=0.0){
    AudioBuffer out;out.sampleRate=sampleRate;out.channels=channels;
    const auto frames=static_cast<SampleIndex>(std::llround(seconds*sampleRate));
    out.interleaved.resize(static_cast<std::size_t>(frames)*channels);
    constexpr double pi=3.14159265358979323846;
    for(SampleIndex frame=0;frame<frames;++frame){
        const float x=static_cast<float>(amplitude*std::sin(2.0*pi*frequency*frame/sampleRate+phase));
        for(int c=0;c<channels;++c)out.interleaved[static_cast<std::size_t>(frame)*channels+c]=x;
    }
    return out;
}

static double peak(const AudioBuffer& audio){
    double p=0.0;for(float x:audio.interleaved)p=std::max(p,std::abs(static_cast<double>(x)));return p;
}

static std::size_t nonZeroSamples(const AudioBuffer& audio){
    return static_cast<std::size_t>(std::count_if(audio.interleaved.begin(),audio.interleaved.end(),[](float x){return x!=0.0f;}));
}

int main(){
    try{
        // BS.1770 K-weighting adds about 0.65 dB at 1 kHz; stereo -20 dBFS peak calibrates near -20.04 LUFS.
        auto calibration=sine(48000,2,6.0,1000.0,0.1);
        const auto m=analyzeMastering(calibration);
        require(std::isfinite(m.integratedLufs),"integrated LUFS must be finite");
        require(std::abs(m.integratedLufs-(-20.04))<0.18,"1 kHz stereo loudness calibration drift");
        require(std::abs(m.momentaryMaxLufs-m.integratedLufs)<0.2,"momentary steady-state calibration");
        require(std::abs(m.shortTermMaxLufs-m.integratedLufs)<0.2,"short-term steady-state calibration");
        require(m.loudnessRangeLu<0.2,"steady tone LRA should be near zero");

        AudioBuffer silence=sine(48000,2,4.0,1000.0,0.0);
        const auto silent=analyzeMastering(silence);
        require(!std::isfinite(silent.integratedLufs),"silence must not report finite programme loudness");

        // Absolute/relative gating should prevent trailing silence from pulling programme loudness down.
        auto gated=sine(48000,2,8.0,1000.0,0.0);
        auto loud=sine(48000,2,4.0,1000.0,0.1);
        std::copy(loud.interleaved.begin(),loud.interleaved.end(),gated.interleaved.begin());
        const auto gatedMetrics=analyzeMastering(gated);
        require(std::abs(gatedMetrics.integratedLufs-m.integratedLufs)<0.35,"integrated loudness gating regression");

        // A 12 kHz tone sampled at 48 kHz with pi/4 phase has ~0.707 sample peaks but ~1.0 inter-sample peak.
        auto intersample=sine(48000,2,1.0,12000.0,0.98,3.14159265358979323846/4.0);
        const auto tp=analyzeMastering(intersample);
        require(tp.maxTruePeakDbTP>tp.maxSamplePeakDbFS+1.5,"4x true-peak estimator did not detect inter-sample peak");
        require(tp.maxTruePeakDbTP<0.5,"true-peak estimator overshot implausibly");

        // EBU R128 target profile: -23 LUFS and <= -1 dBTP when peak headroom allows it.
        auto normalized=sine(48000,2,6.0,1000.0,0.025);
        LoudnessTarget ebu;ebu.targetLufs=-23.0;ebu.maxTruePeakDbTP=-1.0;ebu.toleranceLu=0.2;
        const auto norm=normalizeLoudness(normalized,ebu);
        require(!norm.limitedByTruePeak,"calibration tone should have enough true-peak headroom");
        require(std::abs(norm.after.integratedLufs+23.0)<=0.2,"-23 LUFS normalization target miss");
        require(norm.after.maxTruePeakDbTP<=-1.0+0.05,"normalization exceeded -1 dBTP");
        require(norm.targetWithinTolerance,"normalization report should pass target tolerance");

        // Peak-constrained material must report that true-peak ceiling prevented full loudness gain.
        AudioBuffer impulsive;impulsive.sampleRate=48000;impulsive.channels=2;impulsive.interleaved.assign(48000*4,0.0f);
        for(std::size_t i=0;i<impulsive.interleaved.size();i+=9600)impulsive.interleaved[i]=0.95f;
        const auto limited=normalizeLoudness(impulsive,ebu);
        require(limited.limitedByTruePeak,"true-peak constrained normalization must be explicit");
        require(limited.after.maxTruePeakDbTP<=-1.0+0.06,"true-peak constrained normalization exceeded ceiling");

        // Dither policy: integer PCM reductions default to deterministic TPDF.
        const auto temp=std::filesystem::temp_directory_path()/"flowdaw_phase14";
        std::filesystem::remove_all(temp);std::filesystem::create_directories(temp);
        AudioBuffer zeros;zeros.sampleRate=48000;zeros.channels=2;zeros.interleaved.assign(4096,0.0f);
        WavFile::writePcm16(temp/"zero16.wav",zeros,false);
        WavFile::writePcm16(temp/"dither16.wav",zeros,true);
        WavFile::writePcm24(temp/"zero24.wav",zeros,false);
        WavFile::writePcm24(temp/"dither24.wav",zeros,true);
        const auto zero16=WavFile::read(temp/"zero16.wav"),dither16=WavFile::read(temp/"dither16.wav");
        const auto zero24=WavFile::read(temp/"zero24.wav"),dither24=WavFile::read(temp/"dither24.wav");
        require(nonZeroSamples(zero16)==0&&nonZeroSamples(zero24)==0,"undithered digital silence changed");
        require(nonZeroSamples(dither16)>0&&nonZeroSamples(dither24)>0,"TPDF dither should decorrelate quantized silence");
        require(peak(dither16)<=2.0/32768.0&&peak(dither24)<=2.0/8388608.0,"TPDF dither amplitude exceeds expected LSB scale");

        // Production project export: 24-bit + TPDF + EBU target remains .flow-independent export state.
        Project project;project.sampleRate=48000;project.transport.bpm=120.0;project.name="Phase 14 mastering export";
        SampleAsset sample;sample.name="tone";sample.audio=std::make_shared<AudioBuffer>(sine(48000,2,4.0,1000.0,0.025));sample.path={};
        const Id sampleId=sample.id;project.samples.push_back(sample);
        Track track;track.name="Tone";Clip clip;clip.sampleId=sampleId;clip.sourceLength=sample.audio->frames();clip.lengthTicks=MusicalTime::samplesToTicks(clip.sourceLength,120.0,48000);track.clips.push_back(clip);project.tracks.push_back(track);
        MasterExportOptions options;options.bitDepth=MasterBitDepth::PCM24;options.dither=DitherMode::TPDF;options.normalizeLoudness=true;options.target=ebu;
        const auto report=exportMasteredWav(project,temp/"master.wav",options,0.0);
        require(std::filesystem::is_regular_file(temp/"master.wav"),"mastered WAV export missing");
        const auto delivered=WavFile::read(temp/"master.wav");
        require(delivered.frames()>0&&delivered.sampleRate==48000&&delivered.channels==2,"mastered WAV format regression");
        require(report.normalized&&report.delivered.maxTruePeakDbTP<=-0.94,"master export report true-peak failure");
        require(std::abs(report.delivered.integratedLufs+23.0)<=0.25,"master export loudness target failure");

        std::filesystem::remove_all(temp);
        std::cout<<"FLOWDAW Phase 14.1-14.3 mastering core tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 14.1-14.3 mastering core tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
