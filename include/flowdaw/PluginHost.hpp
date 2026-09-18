#pragma once
#include "flowdaw/Project.hpp"
#include <cstddef>
#include <cstdint>
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
    bool instrument=false;
};

struct PluginMidiEvent {
    int sampleOffset=0;
    std::uint8_t status=0;
    std::uint8_t data1=0;
    std::uint8_t data2=0;
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

    // Phase 8 realtime contract. Existing/offline processors remain source-compatible:
    // realtime execution is opt-in and the engine never falls back to allocating an
    // AudioBuffer inside the device callback.
    virtual bool supportsRealtimeProcessing() const noexcept { return false; }
    virtual int latencySamples() const noexcept { return 0; }
    virtual bool processRealtime(float* interleaved,SampleIndex frames,int channels) noexcept {
        (void)interleaved;(void)frames;(void)channels;return false;
    }
    virtual void resetRealtime() noexcept {}
    virtual bool supportsRealtimeMidiInput() const noexcept { return false; }
    virtual bool processRealtimeMidi(float* interleaved,SampleIndex frames,int channels,
                                     const PluginMidiEvent* events,std::size_t eventCount) noexcept {
        (void)events;(void)eventCount;return processRealtime(interleaved,frames,channels);
    }
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

// Realtime-safe native processors supported directly by the engine. External
// processors use IPluginProcessor::processRealtime only after control-thread
// construction/preparation has succeeded.
void processRealtimeBuiltinSample(const PluginInstance& plugin,float& left,float& right);
}
