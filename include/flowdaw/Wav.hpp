#pragma once
#include "flowdaw/Types.hpp"
#include <filesystem>
#include <vector>
#include <cstdint>

namespace flowdaw {
struct AudioBuffer {
    int sampleRate = 44100;
    int channels = 2;
    std::vector<float> interleaved;
    SampleIndex frames() const { return channels > 0 ? static_cast<SampleIndex>(interleaved.size() / static_cast<std::size_t>(channels)) : 0; }
};

enum class WavIntegerBitDepth { pcm16=16, pcm24=24 };
struct WavPcmOptions {
    WavIntegerBitDepth bitDepth=WavIntegerBitDepth::pcm24;
    bool tpdfDither=true;
    std::uint32_t ditherSeed=0x464c4f57u;
};

class WavFile {
public:
    static AudioBuffer read(const std::filesystem::path& path);
    static void writeFloat32(const std::filesystem::path& path, const AudioBuffer& audio);
    static void writePcm(const std::filesystem::path& path,const AudioBuffer& audio,const WavPcmOptions& options={});
};
}
