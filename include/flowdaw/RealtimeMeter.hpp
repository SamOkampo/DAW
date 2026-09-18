#pragma once
#include "flowdaw/Project.hpp"
#include <atomic>
#include <cstdint>
#include <vector>

namespace flowdaw {

// Sample-domain meter values. These are intentionally not labelled true-peak:
// inter-sample peak estimation is a separate Phase 8 step.
struct AudioMeterReading {
    float samplePeakLeft=0.0f;
    float samplePeakRight=0.0f;
    float rmsLeft=0.0f;
    float rmsRight=0.0f;
};

struct AudioRouteMeterReading {
    Id id=0;
    AudioMeterReading level;
};

struct AudioMeterSnapshot {
    AudioMeterReading master;
    std::vector<AudioRouteMeterReading> tracks;
    std::vector<AudioRouteMeterReading> buses;
};

// The callback computes into locals, then publishes four 32-bit float bit
// patterns. uint32_t atomics are required to be lock-free on supported builds.
class RealtimeMeterState {
public:
    RealtimeMeterState() noexcept;
    void reset() noexcept;
    void process(const float* interleaved,SampleIndex frames,int channels=2) noexcept;
    AudioMeterReading snapshot() const noexcept;

private:
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "FLOWDAW realtime meters require lock-free 32-bit atomics");
    static std::uint32_t encode(float value) noexcept;
    static float decode(std::uint32_t value) noexcept;

    std::atomic<std::uint32_t> peakLeft_{0};
    std::atomic<std::uint32_t> peakRight_{0};
    std::atomic<std::uint32_t> rmsLeft_{0};
    std::atomic<std::uint32_t> rmsRight_{0};
};

} // namespace flowdaw
