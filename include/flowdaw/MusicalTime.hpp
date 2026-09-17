#pragma once
#include "flowdaw/Types.hpp"
#include <cmath>

namespace flowdaw {
struct MusicalPosition {
    int bar = 1;
    int beat = 1;
    int tick = 0;
};

class MusicalTime {
public:
    static Tick toTicks(MusicalPosition p);
    static MusicalPosition fromTicks(Tick ticks);
    static double ticksToSeconds(Tick ticks, double bpm);
    static Tick secondsToTicks(double seconds, double bpm);
    static SampleIndex ticksToSamples(Tick ticks, double bpm, double sampleRate);
    static Tick samplesToTicks(SampleIndex samples, double bpm, double sampleRate);
};
}
