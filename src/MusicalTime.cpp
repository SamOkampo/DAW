#include "flowdaw/MusicalTime.hpp"
#include <algorithm>
#include <stdexcept>

namespace flowdaw {
static void validate(double bpm, double sampleRate = 1.0) {
    if (!(bpm > 0.0) || !(sampleRate > 0.0)) throw std::invalid_argument("bpm/sampleRate must be positive");
}
Tick MusicalTime::toTicks(MusicalPosition p) {
    const auto bars = std::max(0, p.bar - 1);
    const auto beats = std::clamp(p.beat - 1, 0, kBeatsPerBar - 1);
    const auto tick = std::clamp(p.tick, 0, kPPQ - 1);
    return static_cast<Tick>(bars) * kBeatsPerBar * kPPQ + static_cast<Tick>(beats) * kPPQ + tick;
}
MusicalPosition MusicalTime::fromTicks(Tick ticks) {
    ticks = std::max<Tick>(0, ticks);
    MusicalPosition p;
    p.bar = static_cast<int>(ticks / (kBeatsPerBar * kPPQ)) + 1;
    const Tick withinBar = ticks % (kBeatsPerBar * kPPQ);
    p.beat = static_cast<int>(withinBar / kPPQ) + 1;
    p.tick = static_cast<int>(withinBar % kPPQ);
    return p;
}
double MusicalTime::ticksToSeconds(Tick ticks, double bpm) {
    validate(bpm);
    return (static_cast<double>(ticks) / kPPQ) * (60.0 / bpm);
}
Tick MusicalTime::secondsToTicks(double seconds, double bpm) {
    validate(bpm);
    return static_cast<Tick>(std::llround(std::max(0.0, seconds) * bpm / 60.0 * kPPQ));
}
SampleIndex MusicalTime::ticksToSamples(Tick ticks, double bpm, double sampleRate) {
    validate(bpm, sampleRate);
    return static_cast<SampleIndex>(std::llround(ticksToSeconds(ticks, bpm) * sampleRate));
}
Tick MusicalTime::samplesToTicks(SampleIndex samples, double bpm, double sampleRate) {
    validate(bpm, sampleRate);
    return secondsToTicks(static_cast<double>(std::max<SampleIndex>(0, samples)) / sampleRate, bpm);
}
}
