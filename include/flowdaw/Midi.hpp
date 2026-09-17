#pragma once
#include "flowdaw/Types.hpp"
#include <string>
#include <vector>

namespace flowdaw {
double midiNoteFrequency(int pitch);
std::string midiNoteName(int pitch);
Tick snapMidiTick(Tick tick, Tick gridTicks);
bool pitchInScale(int pitch, int rootPitchClass, const std::string& scaleType);
std::vector<int> scalePitchClasses(int rootPitchClass, const std::string& scaleType);
}
