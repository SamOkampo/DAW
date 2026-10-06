#pragma once
#include "flowdaw/Project.hpp"
#include "flowdaw/Loudness.hpp"
#include "flowdaw/Wav.hpp"
#include <filesystem>
#include <memory>
#include <vector>

namespace flowdaw {
class PluginHost;

struct MasterExportOptions {
    WavEncoding encoding=WavEncoding::PCM24;
    DitherMode dither=DitherMode::TPDF;
    bool normalizeLoudness=false;
    double targetLufs=-23.0;
    double maxTruePeakDbtp=-1.0;
    double loudnessToleranceLu=0.5;
    double tailSeconds=2.0;
    std::uint32_t ditherSeed=0x464c4f57u;
};

struct MasterExportReport {
    LoudnessReport before;
    LoudnessReport delivered;
    double appliedGainDb=0.0;
    bool loudnessTargetMet=false;
    bool truePeakLimitMet=false;
};

Tick projectEndTick(const Project& project);
AudioBuffer renderProjectOffline(const Project& project,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
void exportProjectWav(const Project& project,const std::filesystem::path& path,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
MasterExportReport exportProjectMasterWav(const Project&project,const std::filesystem::path&path,const MasterExportOptions&options={},std::shared_ptr<PluginHost>pluginHost={});
std::vector<std::filesystem::path> exportTrackStems(const Project& project,const std::filesystem::path& directory,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
}
