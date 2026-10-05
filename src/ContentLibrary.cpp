#include "flowdaw/ContentLibrary.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace flowdaw {
namespace {
std::string lower(std::string value){
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return value;
}

ContentKind parseKind(const std::string& value){
    if(value=="SAMPLE")return ContentKind::Sample;
    if(value=="INSTRUMENT_PRESET")return ContentKind::InstrumentPreset;
    throw std::runtime_error("Unsupported FLOWDAW content kind: "+value);
}

std::vector<std::string> splitTags(const std::string& value){
    std::vector<std::string> out;
    std::stringstream stream(value);
    std::string tag;
    while(std::getline(stream,tag,',')){
        const auto first=tag.find_first_not_of(" \t");
        if(first==std::string::npos)continue;
        const auto last=tag.find_last_not_of(" \t");
        out.push_back(tag.substr(first,last-first+1));
    }
    return out;
}

void validateEntry(const ContentEntry& entry){
    if(entry.id.empty())throw std::runtime_error("FLOWDAW content entry id is empty");
    if(!isSafeContentRelativePath(entry.relativePath))throw std::runtime_error("Unsafe FLOWDAW content path: "+entry.relativePath.string());
    const auto extension=lower(entry.relativePath.extension().string());
    if(entry.kind==ContentKind::Sample&&extension!=".wav")throw std::runtime_error("Native sample content must be WAV");
    if(entry.kind==ContentKind::InstrumentPreset&&extension!=".flowpreset")throw std::runtime_error("Native instrument presets must use .flowpreset");
}
}

bool isSafeContentRelativePath(const std::filesystem::path& path){
    if(path.empty()||path.is_absolute()||path.has_root_name()||path.has_root_directory())return false;
    for(const auto& part:path){
        const auto value=part.string();
        if(value==".."||value.empty())return false;
    }
    return true;
}

std::filesystem::path resolveContentPath(const std::filesystem::path& root,const ContentEntry& entry){
    validateEntry(entry);
    return (root/entry.relativePath).lexically_normal();
}

ContentManifest loadContentManifest(const std::filesystem::path& path){
    std::ifstream input(path);
    if(!input)throw std::runtime_error("Could not open FLOWDAW content manifest");

    std::string magic;
    ContentManifest manifest;
    input>>magic>>manifest.schemaVersion;
    if(magic!="FLOWDAW_CONTENT"||manifest.schemaVersion!=1)throw std::runtime_error("Unsupported FLOWDAW content manifest");

    std::set<std::string> ids;
    std::string tag;
    while(input>>tag){
        if(tag=="LIBRARY_ID")input>>std::quoted(manifest.libraryId);
        else if(tag=="DISPLAY_NAME")input>>std::quoted(manifest.displayName);
        else if(tag=="LIBRARY_VERSION")input>>manifest.libraryVersion;
        else if(tag=="ENTRY"){
            ContentEntry entry;
            std::string kind,tags;
            std::string relativePath;
            input>>std::quoted(entry.id)>>kind>>std::quoted(relativePath)>>std::quoted(entry.category)>>std::quoted(tags);
            if(!input)throw std::runtime_error("Malformed FLOWDAW content entry");
            entry.kind=parseKind(kind);
            entry.relativePath=relativePath;
            entry.tags=splitTags(tags);
            validateEntry(entry);
            if(!ids.insert(entry.id).second)throw std::runtime_error("Duplicate FLOWDAW content id: "+entry.id);
            manifest.entries.push_back(std::move(entry));
        }else if(tag=="END")break;
        else throw std::runtime_error("Unknown FLOWDAW content manifest field: "+tag);
    }

    if(manifest.libraryId.empty())throw std::runtime_error("FLOWDAW content manifest missing library id");
    if(manifest.displayName.empty())throw std::runtime_error("FLOWDAW content manifest missing display name");
    if(manifest.libraryVersion<1)throw std::runtime_error("FLOWDAW content manifest version must be positive");
    return manifest;
}

}
