#pragma once
#include "flowdaw/Types.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace flowdaw {

struct RealtimeSendLatencyInput {
    Id sendId=0;
    Id busId=0;
    bool enabled=true;
};

struct RealtimeTrackLatencyInput {
    Id trackId=0;
    int pluginLatencySamples=0;
    Id outputBusId=0;
    bool outputEnabled=true;
    std::vector<RealtimeSendLatencyInput> sends;
};

struct RealtimeBusLatencyInput {
    Id busId=0;
    int pluginLatencySamples=0;
};

struct RealtimeRouteIssue {
    Id routeId=0;
    std::string message;
};

struct RealtimeSendPdcPlan {
    Id sendId=0;
    Id busId=0;
    int delaySamples=0;
};

struct RealtimeTrackPdcPlan {
    Id trackId=0;
    int pluginLatencySamples=0;
    Id outputBusId=0;
    bool outputEnabled=true;
    bool outputsToMaster=true;
    int outputDelaySamples=0;
    std::vector<RealtimeSendPdcPlan> sends;
};

struct RealtimeBusPdcPlan {
    Id busId=0;
    bool active=false;
    int inputLatencySamples=0;
    int pluginLatencySamples=0;
    int totalLatencySamples=0;
    int masterDelaySamples=0;
};

struct RealtimePdcPlan {
    int preMasterLatencySamples=0;
    std::vector<RealtimeTrackPdcPlan> tracks;
    std::vector<RealtimeBusPdcPlan> buses;
    std::vector<RealtimeRouteIssue> issues;

    const RealtimeTrackPdcPlan* findTrack(Id id) const noexcept;
    const RealtimeBusPdcPlan* findBus(Id id) const noexcept;
};

// Computes latency alignment for the current FLOWDAW routing topology:
// track -> master OR track -> bus -> master, plus track sends -> bus.
// Buses do not currently route into other buses, so the graph is acyclic.
RealtimePdcPlan buildRealtimePdcPlan(const std::vector<RealtimeTrackLatencyInput>& tracks,
                                     const std::vector<RealtimeBusLatencyInput>& buses);

// Preallocated stereo route storage used by the upcoming AudioEngine routing
// graph. prepare() is control-thread only; clear()/addFrom() never allocate.
class RealtimeStereoRouteBuffer {
public:
    void prepare(SampleIndex maxFrames);
    bool clear(SampleIndex frames) noexcept;
    bool addFrom(const float* source,SampleIndex frames,float gain=1.0f) noexcept;
    float* data() noexcept { return storage_.data(); }
    const float* data() const noexcept { return storage_.data(); }
    SampleIndex capacityFrames() const noexcept { return capacityFrames_; }
    SampleIndex activeFrames() const noexcept { return activeFrames_; }

private:
    std::vector<float> storage_;
    SampleIndex capacityFrames_=0;
    SampleIndex activeFrames_=0;
};

} // namespace flowdaw
