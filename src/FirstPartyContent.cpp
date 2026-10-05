#include "flowdaw/FirstPartyContent.hpp"
#include "flowdaw/TimeStretch.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <stdexcept>

namespace flowdaw {

bool isFirstPartyContentKey(const std::string& key){
    return key.rfind(kFirstPartyContentPrefix,0)==0&&key.size()>std::char_traits<char>::length(kFirstPartyContentPrefix);
}

std::string firstPartyContentId(const std::string& key){
    if(!isFirstPartyContentKey(key))return{};
    return key.substr(std::char_traits<char>::length(kFirstPartyContentPrefix));
}

std::string makeFirstPartyContentKey(const std::string& id){
    if(id.empty())throw std::invalid_argument("First-party content id is empty");
    return std::string(kFirstPartyContentPrefix)+id;
}

const ContentEntry* findContentEntryById(const ContentManifest& manifest,const std::string& id,ContentKind kind){
    const auto it=std::find_if(manifest.entries.begin(),manifest.entries.end(),[&](const auto& entry){
        return entry.kind==kind&&entry.id==id;
    });
    return it==manifest.entries.end()?nullptr:&*it;
}

std::size_t hydrateFirstPartyContent(Project& project,const std::filesystem::path& root){
    if(root.empty())return 0;
    const auto manifest=loadContentManifest(root/"flow-core.manifest");
    std::size_t hydrated=0;
    for(auto& sample:project.samples){
        if(sample.sourceSampleId!=0||sample.audio||!isFirstPartyContentKey(sample.nativeKey))continue;
        const auto id=firstPartyContentId(sample.nativeKey);
        const auto* entry=findContentEntryById(manifest,id,ContentKind::Sample);
        if(!entry)continue;
        const auto path=resolveContentPath(root,*entry);
        std::error_code ec;
        if(!std::filesystem::is_regular_file(path,ec))continue;
        auto audio=std::make_shared<AudioBuffer>(WavFile::read(path));
        if(audio->frames()<=0)continue;
        sample.audio=std::move(audio);
        ++hydrated;
    }
    for(auto& derived:project.samples){
        if(derived.sourceSampleId==0||derived.audio)continue;
        auto* source=project.findSample(derived.sourceSampleId);
        if(source&&source->audio&&derived.timeRatio>=0.5&&derived.timeRatio<=2.0){
            derived.audio=std::make_shared<AudioBuffer>(timeStretchWsola(*source->audio,derived.timeRatio));
            ++hydrated;
        }
    }
    return hydrated;
}

NativeInstrumentPreset loadFirstPartyInstrumentPreset(const std::filesystem::path& root,const std::string& id){
    if(root.empty())throw std::runtime_error("FLOW Core root is unavailable");
    const auto manifest=loadContentManifest(root/"flow-core.manifest");
    const auto* entry=findContentEntryById(manifest,id,ContentKind::InstrumentPreset);
    if(!entry)throw std::runtime_error("Unknown FLOW Core preset id: "+id);
    return loadNativeInstrumentPreset(resolveContentPath(root,*entry));
}

}
