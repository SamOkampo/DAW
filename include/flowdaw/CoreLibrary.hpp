#pragma once
#include <cstddef>
#include <filesystem>

namespace flowdaw {

struct CoreLibraryBuildSummary {
    std::size_t sampleCount=0;
    int sampleRate=0;
};

CoreLibraryBuildSummary writeFlowCoreLibrary(const std::filesystem::path& root,int sampleRate=48000);

}
