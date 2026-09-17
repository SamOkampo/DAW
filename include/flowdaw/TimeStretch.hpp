#pragma once
#include "flowdaw/Wav.hpp"
namespace flowdaw {
AudioBuffer timeStretchWsola(const AudioBuffer& input,double ratio);
AudioBuffer matchBpmWsola(const AudioBuffer& input,double sourceBpm,double targetBpm);
}
