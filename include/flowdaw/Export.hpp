#pragma once
#include "flowdaw/Project.hpp"
#include "flowdaw/Mastering.hpp"
#include "flowdaw/Wav.hpp"
#include <filesystem>
#include <memory>
#include <vector>

namespace flowdaw {
class PluginHost;

enum class MasteringExportEncoding { float32, pcm24, pcm16 };

struct MasteringExportOptions {
    MasteringExportEncoding encoding=MasteringExportEncoding::pcm24;
    bool tpdfDither=true;
    bool normalizeToTarget=true;
    MasteringTarget target{};
};

struct MasteringExportResult {
    MasteringAnalysis before;
    MasteringAnalysis after;
    MasteringCompliance compliance;
    double appliedGainDb=0.0;
};

Tick projectEndTick(const Project& project);
AudioBuffer renderProjectOffline(const Project& project,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
void exportProjectWav(const Project& project,const std::filesystem::path& path,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
MasteringExportResult exportMasteringWav(const Project& project,const std::filesystem::path& path,const MasteringExportOptions& options={},double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
std::vector<std::filesystem::path> exportTrackStems(const Project& project,const std::filesystem::path& directory,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
}
