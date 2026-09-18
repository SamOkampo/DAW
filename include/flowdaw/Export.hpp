#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <vector>

namespace flowdaw {
class PluginHost;
Tick projectEndTick(const Project& project);
AudioBuffer renderProjectOffline(const Project& project,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
void exportProjectWav(const Project& project,const std::filesystem::path& path,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
std::vector<std::filesystem::path> exportTrackStems(const Project& project,const std::filesystem::path& directory,double tailSeconds=2.0,std::shared_ptr<PluginHost> pluginHost={});
}
