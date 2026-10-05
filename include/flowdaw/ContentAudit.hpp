#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace flowdaw {

struct ContentAuditReport {
    bool passed=false;
    std::size_t sampleCount=0;
    std::size_t presetCount=0;
    std::vector<std::string> issues;
};

ContentAuditReport auditFlowCoreLibrary(const std::filesystem::path& root);

}
