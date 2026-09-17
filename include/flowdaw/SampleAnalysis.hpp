#pragma once
#include "flowdaw/Wav.hpp"
#include <vector>
namespace flowdaw {
struct BpmEstimate { double bpm=0.0; double confidence=0.0; bool valid() const { return bpm>0.0&&confidence>0.0; } };
struct SliceRange { SampleIndex startFrame=0; SampleIndex endFrame=0; };
std::vector<SampleIndex> detectTransients(const AudioBuffer& audio,double sensitivity=1.5,double minSpacingSeconds=0.06);
BpmEstimate detectBpm(const AudioBuffer& audio,double minBpm=60.0,double maxBpm=200.0);
std::vector<SliceRange> makeEqualSlices(const AudioBuffer& audio,int count);
}
