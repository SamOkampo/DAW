#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace flowdaw {
struct AudioDeviceSettings {
    int preferredSampleRate=48000;
    unsigned long bufferSize=256;
    std::string inputDevice;
    std::string outputDevice;
    bool monitorByDefault=false;
};

struct AppSettings {
    int version=2;
    AudioDeviceSettings audio;
    int autosaveSeconds=120;
    bool restoreLastSession=true;
    std::filesystem::path lastProjectPath;
    std::vector<std::filesystem::path> pluginRoots;
    std::vector<std::filesystem::path> sampleRoots;
    std::vector<std::filesystem::path> favoriteSamples;
    std::vector<std::filesystem::path> recentSamples;
};

std::filesystem::path defaultSettingsDirectory();
AppSettings defaultAppSettings();
void saveAppSettings(const AppSettings& settings,const std::filesystem::path& path);
AppSettings loadAppSettings(const std::filesystem::path& path);
unsigned long sanitizeBufferSize(unsigned long value);
}
