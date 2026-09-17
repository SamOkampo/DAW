#pragma once
#include "flowdaw/Project.hpp"

namespace flowdaw {
AudioBuffer renderNativeInstrumentNote(const InstrumentState& instrument,int midiPitch,float velocity,SampleIndex noteFrames,int sampleRate,double bpm);
}
