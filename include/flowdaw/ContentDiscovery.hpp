#pragma once
#include <filesystem>

namespace flowdaw {

bool isValidFlowCoreRoot(const std::filesystem::path& root);
std::filesystem::path discoverFlowCoreRoot(
    const std::filesystem::path& executablePath,
    const std::filesystem::path& buildFallback={});

}
