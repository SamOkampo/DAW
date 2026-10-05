#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace flowdaw {

struct NativeInstrumentPreset {
    int schemaVersion=1;
    std::string id;
    std::string name;
    std::string category;
    InstrumentState instrument;
};

struct NativeEffectPreset {
    int schemaVersion=1;
    std::string id;
    std::string name;
    std::string category;
    std::string pluginIdentifier;
    float wet=1.0f;
    std::vector<PluginParameter> parameters;
};

bool isSupportedNativeInstrumentType(const std::string& type);
NativeInstrumentPreset loadNativeInstrumentPreset(const std::filesystem::path& path);
void saveNativeInstrumentPreset(const NativeInstrumentPreset& preset,const std::filesystem::path& path);

bool isSupportedNativeEffectIdentifier(const std::string& identifier);
NativeEffectPreset loadNativeEffectPreset(const std::filesystem::path& path);
void saveNativeEffectPreset(const NativeEffectPreset& preset,const std::filesystem::path& path);
std::vector<NativeEffectPreset> builtinNativeEffectPresets();
PluginInstance instantiateNativeEffectPreset(const NativeEffectPreset& preset);

}
