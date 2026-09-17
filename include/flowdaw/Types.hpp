#pragma once
#include <cstdint>
#include <string>

namespace flowdaw {
using SampleIndex = std::int64_t;
using Tick = std::int64_t;
using Id = std::uint64_t;

constexpr int kPPQ = 960;
constexpr int kBeatsPerBar = 4;

inline Id nextId() {
    static std::uint64_t counter = 1;
    return counter++;
}

struct StereoFrame {
    float left = 0.0f;
    float right = 0.0f;
};
}
