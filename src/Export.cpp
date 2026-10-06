#include "flowdaw/Export.hpp"
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/PluginHost.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cmath>

namespace flowdaw {
Tick projectEndTick(const Project&project){
    Tick end=0;
    for(auto const&t:project.tracks){
        for(auto const&c:t.clips)end=std::max(end,c.startTick+std::max<Tick>(0,c.lengthTicks));
        for(auto const&pc:t.patternClips){auto const*p=project.findPattern(pc.patternId);if(p)end=std::max(end,pc.startTick+p->lengthTicks()*std::max(1,pc.repeats));}
        for(auto const&take:t.takes)end=std::max(end,take.startTick+std::max<Tick>(0,take.lengthTicks));
    }
    return end;
}
AudioBuffer renderProjectOffline(const Project&project,double tailSeconds,std::shared_ptr<PluginHost>pluginHost){
    AudioEngine engine;
    engine.configureExternalDevice(project.sampleRate,512,false);
    if(pluginHost)engine.setPluginHost(pluginHost);
    engine.publish(project);
    const Tick end=projectEndTick(project);
    SampleIndex frames=MusicalTime::ticksToSamples(end,project.transport.bpm,project.sampleRate);
    frames+=static_cast<SampleIndex>(std::max(0.0,tailSeconds)*project.sampleRate);
    return engine.renderOffline(frames);
}
void exportProjectWav(const Project&project,const std::filesystem::path&path,double tailSeconds,std::shared_ptr<PluginHost>pluginHost){
    auto audio=renderProjectOffline(project,tailSeconds,pluginHost);
    std::filesystem::create_directories(path.parent_path().empty()?std::filesystem::path("."):path.parent_path());
    WavFile::writeFloat32(path,audio);
}
MasterExportReport exportProjectMasterWav(const Project&project,const std::filesystem::path&path,const MasterExportOptions&options,std::shared_ptr<PluginHost>pluginHost){
    auto audio=renderProjectOffline(project,options.tailSeconds,pluginHost);
    MasterExportReport report;report.before=MasteringAnalyzer::analyze(audio);
    if(options.normalizeLoudness&&!report.before.silence&&std::isfinite(report.before.integratedLufs)){
        const double desired=options.targetLufs-report.before.integratedLufs;
        const double truePeakHeadroom=std::isfinite(report.before.truePeakDbtp)?options.maxTruePeakDbtp-report.before.truePeakDbtp:desired;
        report.appliedGainDb=std::min(desired,truePeakHeadroom);
        const double gain=std::pow(10.0,report.appliedGainDb/20.0);
        for(auto&sample:audio.interleaved)sample=static_cast<float>(static_cast<double>(sample)*gain);
    }
    std::filesystem::create_directories(path.parent_path().empty()?std::filesystem::path("."):path.parent_path());
    WavWriteOptions wav;wav.encoding=options.encoding;wav.dither=options.encoding==WavEncoding::Float32?DitherMode::None:options.dither;wav.ditherSeed=options.ditherSeed;
    WavFile::write(path,audio,wav);
    const auto deliveredAudio=WavFile::read(path);
    report.delivered=MasteringAnalyzer::analyze(deliveredAudio);
    report.loudnessTargetMet=!options.normalizeLoudness||(std::isfinite(report.delivered.integratedLufs)&&std::abs(report.delivered.integratedLufs-options.targetLufs)<=options.loudnessToleranceLu);
    report.truePeakLimitMet=!std::isfinite(report.delivered.truePeakDbtp)||report.delivered.truePeakDbtp<=options.maxTruePeakDbtp+0.05;
    return report;
}
std::vector<std::filesystem::path> exportTrackStems(const Project&project,const std::filesystem::path&directory,double tailSeconds,std::shared_ptr<PluginHost>pluginHost){
    std::filesystem::create_directories(directory);std::vector<std::filesystem::path> paths;paths.reserve(project.tracks.size());
    for(std::size_t i=0;i<project.tracks.size();++i){
        Project stem=project;
        for(std::size_t j=0;j<stem.tracks.size();++j)stem.tracks[j].mixer.mute=j!=i;
        std::string name=project.tracks[i].name.empty()?"Track_"+std::to_string(i+1):project.tracks[i].name;
        for(char&c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='-'&&c!='_')c='_';
        auto path=directory/(std::to_string(i+1)+"_"+name+".wav");
        exportProjectWav(stem,path,tailSeconds,pluginHost);paths.push_back(path);
    }
    return paths;
}
}
