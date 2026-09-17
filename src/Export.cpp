#include "flowdaw/Export.hpp"
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>

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
AudioBuffer renderProjectOffline(const Project&project,double tailSeconds){
    AudioEngine engine;engine.publish(project);const Tick end=projectEndTick(project);SampleIndex frames=MusicalTime::ticksToSamples(end,project.transport.bpm,project.sampleRate);frames+=static_cast<SampleIndex>(std::max(0.0,tailSeconds)*project.sampleRate);return engine.renderOffline(frames);
}
void exportProjectWav(const Project&project,const std::filesystem::path&path,double tailSeconds){auto audio=renderProjectOffline(project,tailSeconds);std::filesystem::create_directories(path.parent_path().empty()?std::filesystem::path("."):path.parent_path());WavFile::writeFloat32(path,audio);}
std::vector<std::filesystem::path> exportTrackStems(const Project&project,const std::filesystem::path&directory,double tailSeconds){
    std::filesystem::create_directories(directory);std::vector<std::filesystem::path> paths;paths.reserve(project.tracks.size());
    for(std::size_t i=0;i<project.tracks.size();++i){Project stem=project;for(std::size_t j=0;j<stem.tracks.size();++j)stem.tracks[j].mixer.mute=j!=i;std::string name=project.tracks[i].name.empty()?"Track_"+std::to_string(i+1):project.tracks[i].name;for(char&c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='-'&&c!='_')c='_';auto path=directory/(std::to_string(i+1)+"_"+name+".wav");exportProjectWav(stem,path,tailSeconds);paths.push_back(path);}
    return paths;
}
}
