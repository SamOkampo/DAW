#pragma once
#include "flowdaw/ContentLibrary.hpp"
#include "flowdaw/NativePresets.hpp"
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <string>

namespace flowdaw {

inline constexpr const char* kFirstPartyContentPrefix="content:";

bool isFirstPartyContentKey(const std::string& key);
std::string firstPartyContentId(const std::string& key);
std::string makeFirstPartyContentKey(const std::string& id);
const ContentEntry* findContentEntryById(const ContentManifest& manifest,const std::string& id,ContentKind kind);
std::size_t hydrateFirstPartyContent(Project& project,const std::filesystem::path& root);
NativeInstrumentPreset loadFirstPartyInstrumentPreset(const std::filesystem::path& root,const std::string& id);

}
