#pragma once
#include "flowdaw/Types.hpp"
#include <filesystem>
#include <vector>

namespace flowdaw {
struct AudioBuffer {
    int sampleRate = 44100;
    int channels = 2;
    std::vector<float> interleaved;
    SampleIndex frames() const { return channels > 0 ? static_cast<SampleIndex>(interleaved.size() / static_cast<std::size_t>(channels)) : 0; }
};

class WavFile {
public:
    static AudioBuffer read(const std::filesystem::path& path);
    static void writeFloat32(const std::filesystem::path& path, const AudioBuffer& audio);
};
}
