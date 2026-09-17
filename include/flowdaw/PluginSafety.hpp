#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace flowdaw {
struct PluginSafetyRecord {
    std::string identifier;
    int failures=0;
    bool quarantined=false;
    std::string lastError;
};

class PluginSafetyRegistry {
public:
    explicit PluginSafetyRegistry(int quarantineThreshold=3):threshold_(quarantineThreshold){}
    void noteFailure(const std::string& identifier,const std::string& error);
    void noteSuccess(const std::string& identifier);
    bool isQuarantined(const std::string& identifier) const;
    void clearQuarantine(const std::string& identifier);
    const std::vector<PluginSafetyRecord>& records() const { return records_; }
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);
private:
    int threshold_=3;
    std::vector<PluginSafetyRecord> records_;
    PluginSafetyRecord* find(const std::string& identifier);
    const PluginSafetyRecord* find(const std::string& identifier) const;
};
}
