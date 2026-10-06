#include "flowdaw/Export.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/PluginHost.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Wav.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static AudioBuffer sourceTone(){
    AudioBuffer b;b.sampleRate=48000;b.channels=2;b.interleaved.resize(48000*8*2);
    constexpr double pi=3.14159265358979323846;
    for(int i=0;i<48000*8;++i){float x=0.16f*static_cast<float>(std::sin(2*pi*440.0*i/48000.0));b.interleaved[static_cast<std::size_t>(i)*2]=x;b.interleaved[static_cast<std::size_t>(i)*2+1]=x;}
    return b;
}
static int wavBits(const std::filesystem::path&p){
    std::ifstream f(p,std::ios::binary);if(!f)return 0;f.seekg(34);unsigned char b[2]{};f.read(reinterpret_cast<char*>(b),2);return static_cast<int>(b[0]|(b[1]<<8));
}
int main(){
 try{
    const auto dir=std::filesystem::temp_directory_path()/"flowdaw_phase14_golden";std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);
    Project project;project.name="Phase14 Golden";project.sampleRate=48000;project.transport.bpm=120.0;
    SampleAsset asset;asset.name="Mastering Tone";asset.audio=std::make_shared<AudioBuffer>(sourceTone());const Id sid=asset.id;project.samples.push_back(asset);
    Track track;track.name="Music";Clip clip;clip.sampleId=sid;clip.sourceLength=asset.audio->frames();clip.lengthTicks=MusicalTime::samplesToTicks(asset.audio->frames(),project.transport.bpm,project.sampleRate);track.clips.push_back(clip);
    auto eq=makeBuiltinPlugin("flow.eq");setPluginParameter(eq,"band4_freq",2500.0f);setPluginParameter(eq,"band4_gain_db",1.0f);track.mixer.plugins.push_back(eq);
    auto comp=makeBuiltinPlugin("flow.compressor");setPluginParameter(comp,"threshold_db",-20.0f);setPluginParameter(comp,"ratio",2.0f);track.mixer.plugins.push_back(comp);project.tracks.push_back(track);
    auto limiter=makeBuiltinPlugin("flow.limiter");setPluginParameter(limiter,"ceiling_db",-1.0f);setPluginParameter(limiter,"lookahead_ms",3.0f);project.master.plugins.push_back(limiter);

    const auto flowPath=dir/"mastering.flow";ProjectSerializer::save(project,flowPath);const auto reopened=ProjectSerializer::load(flowPath,false);
    require(reopened.formatVersion==11,"Phase 14 golden path changed .flow format");
    require(reopened.tracks.size()==1&&reopened.tracks.front().mixer.plugins.size()==2&&reopened.master.plugins.size()==1,"native mastering chain did not survive save/reopen");

    MasterExportOptions options;options.encoding=WavEncoding::PCM24;options.dither=DitherMode::TPDF;options.normalizeLoudness=true;options.targetLufs=-23.0;options.maxTruePeakDbtp=-1.0;options.loudnessToleranceLu=0.5;options.tailSeconds=0.0;options.ditherSeed=1405;
    const auto path=dir/"master-r128.wav";const auto report=exportProjectMasterWav(reopened,path,options);
    require(std::filesystem::is_regular_file(path)&&std::filesystem::file_size(path)>44,"golden master missing");
    require(wavBits(path)==24,"golden master is not PCM24");
    require(report.loudnessTargetMet,"golden master did not meet loudness target");
    require(report.truePeakLimitMet,"golden master exceeded true-peak ceiling");
    require(std::abs(report.delivered.integratedLufs+23.0)<=0.5,"golden master outside -23 LUFS tolerance");
    require(report.delivered.truePeakDbtp<=-0.95,"golden master exceeds -1 dBTP tolerance");
    const auto decoded=WavFile::read(path);require(decoded.frames()>0&&!MasteringAnalyzer::analyze(decoded).silence,"golden master re-read failed");
    std::filesystem::remove_all(dir);
    std::cout<<"FLOWDAW Phase 14.5 mastering golden path: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FLOWDAW Phase 14.5 mastering golden path: FAIL: "<<e.what()<<"\n";return 1;}
}
