#pragma once
#include "flowdaw/Wav.hpp"
#include <utility>
#include <vector>
namespace flowdaw {
using PeakPair = std::pair<float,float>;
std::vector<PeakPair> buildWaveform(const AudioBuffer& audio, std::size_t columns);
}
