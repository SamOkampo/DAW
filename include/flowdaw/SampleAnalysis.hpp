#pragma once
#include "flowdaw/Wav.hpp"
#include <vector>
namespace flowdaw {
struct BpmEstimate { double bpm=0.0; double confidence=0.0; bool valid() const { return bpm>0.0&&confidence>0.0; } };
struct BeatGridEstimate {
    double bpm=0.0;
    double confidence=0.0;
    SampleIndex firstBeatFrame=0;
    SampleIndex downbeatFrame=0; // 4/4 baseline candidate, not a semantic guarantee.
    double downbeatConfidence=0.0;
    bool valid() const { return bpm>0.0&&confidence>0.0; }
};
struct SliceRange { SampleIndex startFrame=0; SampleIndex endFrame=0; };
std::vector<SampleIndex> detectTransients(const AudioBuffer& audio,double sensitivity=1.5,double minSpacingSeconds=0.06);
BpmEstimate detectBpm(const AudioBuffer& audio,double minBpm=60.0,double maxBpm=200.0);
BeatGridEstimate estimateBeatGrid(const AudioBuffer& audio,double minBpm=60.0,double maxBpm=200.0);
std::vector<SliceRange> makeEqualSlices(const AudioBuffer& audio,int count);
std::vector<SliceRange> makeBeatSlices(const AudioBuffer& audio,const BeatGridEstimate& grid,int beatsPerSlice=1);
}
