#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw {
struct PluginDescriptor {
    std::string format;
    std::string identifier;
    std::string name;
    std::string vendor;
    std::string category="effect";
    std::filesystem::path path;
    bool builtin=false;
};

float pluginParameterValue(const PluginInstance& plugin,const std::string& id,float fallback);
void setPluginParameter(PluginInstance& plugin,const std::string& id,float value);
PluginInstance makeBuiltinPlugin(const std::string& identifier);
std::vector<PluginDescriptor> builtinPluginDescriptors();
std::vector<PluginDescriptor> scanPluginPaths(const std::vector<std::filesystem::path>& roots);

class IPluginProcessor {
public:
    virtual ~IPluginProcessor()=default;
    virtual bool prepare(int sampleRate,int channels,std::string& error)=0;
    virtual void setState(const std::string& state)=0;
    virtual std::string state() const=0;
    virtual void process(AudioBuffer& buffer)=0;
};

class IExternalPluginBackend {
public:
    virtual ~IExternalPluginBackend()=default;
    virtual bool supports(const std::string& format) const=0;
    virtual std::unique_ptr<IPluginProcessor> create(const PluginInstance& plugin,std::string& error)=0;
};

class PluginHost {
public:
    void registerBackend(std::shared_ptr<IExternalPluginBackend> backend);
    std::unique_ptr<IPluginProcessor> createProcessor(const PluginInstance& plugin,std::string& error) const;
    bool process(AudioBuffer& buffer,PluginInstance& plugin,std::string& error) const;
    bool processChain(AudioBuffer& buffer,std::vector<PluginInstance>& plugins,std::string& error) const;
private:
    std::vector<std::shared_ptr<IExternalPluginBackend>> backends_;
};

// Realtime-safe master processors supported directly by the bootstrap engine.
// External VST3/AU processors are intentionally instantiated only by a registered backend.
void processRealtimeBuiltinSample(const PluginInstance& plugin,float& left,float& right);
}
