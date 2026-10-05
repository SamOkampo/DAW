#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <string>

namespace flowdaw {

struct NativeInstrumentPreset {
    int schemaVersion=1;
    std::string id;
    std::string name;
    std::string category;
    InstrumentState instrument;
};

bool isSupportedNativeInstrumentType(const std::string& type);
NativeInstrumentPreset loadNativeInstrumentPreset(const std::filesystem::path& path);
void saveNativeInstrumentPreset(const NativeInstrumentPreset& preset,const std::filesystem::path& path);

}
