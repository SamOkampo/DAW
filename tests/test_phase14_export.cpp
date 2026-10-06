#include "flowdaw/Export.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Wav.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}

static AudioBuffer tone(int sr,int frames,float amp){
    AudioBuffer b;b.sampleRate=sr;b.channels=2;b.interleaved.resize(static_cast<std::size_t>(frames)*2);
    constexpr double pi=3.14159265358979323846;
    for(int i=0;i<frames;++i){float x=amp*static_cast<float>(std::sin(2*pi*1000.0*i/sr));b.interleaved[static_cast<std::size_t>(i)*2]=x;b.interleaved[static_cast<std::size_t>(i)*2+1]=x;}
    return b;
}
static Project projectWithTone(){
    Project p;p.name="Phase14";p.sampleRate=48000;p.transport.bpm=120.0;
    SampleAsset a;a.name="tone";a.audio=tone(48000,240000,0.1f);const Id id=a.id;p.samples.push_back(a);
    Track t;t.name="Tone";Clip c;c.sampleId=id;c.sourceLength=a.audio.frames();c.lengthTicks=MusicalTime::samplesToTicks(a.audio.frames(),120.0,48000);t.clips.push_back(c);p.tracks.push_back(t);
    return p;
}
int main(){
 try{
    const auto dir=std::filesystem::temp_directory_path()/"flowdaw_phase14_export";std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);
    AudioBuffer zero;zero.sampleRate=48000;zero.channels=2;zero.interleaved.assign(4096,0.0f);
    WavFile::write(dir/"zero-none.wav",zero,{WavEncoding::PCM16,DitherMode::None,1});
    WavFile::write(dir/"zero-dither.wav",zero,{WavEncoding::PCM16,DitherMode::TPDF,1});
    const auto none=WavFile::read(dir/"zero-none.wav");const auto dith=WavFile::read(dir/"zero-dither.wav");
    bool anyDither=false;for(float x:none.interleaved)require(x==0.0f,"undithered digital silence changed");for(float x:dith.interleaved)if(x!=0.0f)anyDither=true;
    require(anyDither,"TPDF dither should decorrelate quantization at digital silence");

    auto src=tone(48000,48000,0.333333f);
    WavFile::write(dir/"24.wav",src,{WavEncoding::PCM24,DitherMode::TPDF,123});
    WavFile::write(dir/"16.wav",src,{WavEncoding::PCM16,DitherMode::TPDF,123});
    const auto r24=WavFile::read(dir/"24.wav"),r16=WavFile::read(dir/"16.wav");
    require(r24.frames()==src.frames()&&r16.frames()==src.frames(),"integer WAV roundtrip length");
    double err24=0,err16=0;for(std::size_t i=0;i<src.interleaved.size();++i){err24+=std::abs(src.interleaved[i]-r24.interleaved[i]);err16+=std::abs(src.interleaved[i]-r16.interleaved[i]);}
    require(err24<err16*0.02,"PCM24 should quantize far more accurately than PCM16");

    const auto project=projectWithTone();
    MasterExportOptions options;options.encoding=WavEncoding::PCM24;options.dither=DitherMode::TPDF;options.normalizeLoudness=true;options.targetLufs=-23.0;options.maxTruePeakDbtp=-1.0;options.loudnessToleranceLu=0.6;options.tailSeconds=0.0;options.ditherSeed=77;
    const auto report=exportProjectMasterWav(project,dir/"master24.wav",options);
    require(std::filesystem::is_regular_file(dir/"master24.wav"),"master export missing");
    require(std::isfinite(report.before.integratedLufs)&&std::isfinite(report.delivered.integratedLufs),"master report loudness finite");
    require(report.truePeakLimitMet,"master export exceeded requested true-peak ceiling");
    require(report.loudnessTargetMet,"master export failed requested loudness target");
    require(std::abs(report.delivered.integratedLufs+23.0)<=0.6,"delivered R128 target outside tolerance");

    MasterExportOptions floatOptions;floatOptions.encoding=WavEncoding::Float32;floatOptions.dither=DitherMode::TPDF;floatOptions.tailSeconds=0.0;
    const auto floatReport=exportProjectMasterWav(project,dir/"master-float.wav",floatOptions);
    require(std::isfinite(floatReport.delivered.integratedLufs),"float master report");
    std::filesystem::remove_all(dir);
    std::cout<<"FLOWDAW Phase 14.2 export/dither tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FLOWDAW Phase 14.2 export/dither tests: FAIL: "<<e.what()<<"\n";return 1;}
}
