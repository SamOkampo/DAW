#pragma once
#include "flowdaw/Wav.hpp"
#include <string>
namespace flowdaw {
AudioBuffer makeNativeDrum(const std::string& key,int sampleRate=48000);
}
