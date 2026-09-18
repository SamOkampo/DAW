#pragma once
#include "flowdaw/PluginHost.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw {

struct RealtimePluginIssue {
    Id pluginId=0;
    std::string message;
};

// Fixed-latency interleaved delay used by Phase 8 PDC. Storage is allocated on
// the control thread in prepare(); addDelayed() performs no allocation or lock.
class RealtimeDelayLine {
public:
    void prepare(int delaySamples,int channels);
    void reset() noexcept;
    int delaySamples() const noexcept { return delaySamples_; }
    int channels() const noexcept { return channels_; }
    void addDelayed(const float* source,float* destination,SampleIndex frames,float gain=1.0f) noexcept;
private:
    std::vector<float> storage_;
    int delaySamples_=0;
    int channels_=0;
    int writeFrame_=0;
};

// Prepared plugin chain for the device callback. Processor creation, state
// restore, prepare() and scratch allocation all happen in prepare(), never from
// process(). Unsupported realtime processors are omitted with an issue rather
// than being instantiated from the callback.
class RealtimePluginChain {
public:
    bool prepare(const std::vector<PluginInstance>& plugins,
                 const PluginHost* host,
                 int sampleRate,
                 int channels,
                 SampleIndex maxBlockFrames,
                 std::vector<RealtimePluginIssue>* issues=nullptr);
    void process(float* interleaved,SampleIndex frames) noexcept;
    void reset() noexcept;
    int latencySamples() const noexcept { return latencySamples_; }
    std::size_t size() const noexcept { return slots_.size(); }
    bool empty() const noexcept { return slots_.empty(); }

private:
    struct Slot {
        PluginInstance config;
        bool builtin=false;
        std::unique_ptr<IPluginProcessor> processor;
        int latencySamples=0;
        std::vector<float> inputScratch;
        std::vector<float> delayedDryScratch;
        RealtimeDelayLine dryDelay;
    };

    std::vector<Slot> slots_;
    int channels_=2;
    SampleIndex maxBlockFrames_=1;
    int latencySamples_=0;
};

// Dedicated source instrument slot. Unlike RealtimePluginChain, this processor
// receives MIDI and renders into a zeroed track-local buffer before mixer
// inserts. Construction/state restore/prepare are control-thread only.
class RealtimePluginInstrument {
public:
    bool prepare(const PluginInstance& plugin,
                 const PluginHost* host,
                 int sampleRate,
                 int channels,
                 std::vector<RealtimePluginIssue>* issues=nullptr);
    bool process(float* interleaved,SampleIndex frames,
                 const PluginMidiEvent* events,std::size_t eventCount) noexcept;
    void reset() noexcept;
    bool ready() const noexcept { return processor_!=nullptr; }
    int latencySamples() const noexcept { return latencySamples_; }

private:
    PluginInstance config_;
    std::unique_ptr<IPluginProcessor> processor_;
    int channels_=2;
    int latencySamples_=0;
};

} // namespace flowdaw
