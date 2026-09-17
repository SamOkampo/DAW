#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <string>

namespace flowdaw {
struct RecoveryInfo {
    bool available=false;
    std::filesystem::path originalProject;
    std::filesystem::path autosaveProject;
    std::uint64_t generation=0;
};

class SessionRecovery {
public:
    explicit SessionRecovery(std::filesystem::path root);
    RecoveryInfo inspect() const;
    void beginSession(const std::filesystem::path& originalProject);
    void autosave(const Project& project,const std::filesystem::path& originalProject);
    Project loadRecovered(bool loadAudio=true) const;
    void markCleanExit();
    const std::filesystem::path& root() const { return root_; }
private:
    std::filesystem::path root_,marker_,autosave_;
    std::uint64_t generation_=0;
    void writeMarker(const std::filesystem::path& originalProject);
};
}
