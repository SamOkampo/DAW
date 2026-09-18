#pragma once
#include "flowdaw/Project.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace flowdaw {

// Meter values published from the realtime callback. truePeak* is a 4x
// cubic inter-sample estimate; it is intentionally not advertised as a
// standards-certified BS.1770/EBU true-peak implementation.
struct AudioMeterReading {
    float samplePeakLeft=0.0f;
    float samplePeakRight=0.0f;
    float truePeakLeft=0.0f;
    float truePeakRight=0.0f;
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

// The callback computes into locals, then publishes 32-bit float bit patterns.
// uint32_t atomics are required to be lock-free on supported builds. The
// inter-sample estimator keeps only three samples of history per channel and
// performs no allocation or locking from process().
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
    static float interpolate4x(float p0,float p1,float p2,float p3,float t) noexcept;
    static void pushHistory(std::array<float,3>& history,float value) noexcept;

    std::atomic<std::uint32_t> peakLeft_{0};
    std::atomic<std::uint32_t> peakRight_{0};
    std::atomic<std::uint32_t> truePeakLeft_{0};
    std::atomic<std::uint32_t> truePeakRight_{0};
    std::atomic<std::uint32_t> rmsLeft_{0};
    std::atomic<std::uint32_t> rmsRight_{0};
    std::array<float,3> historyLeft_{};
    std::array<float,3> historyRight_{};
    unsigned historyCount_=0;
};

} // namespace flowdaw
