#pragma once
#include "flowdaw/Types.hpp"
#include <filesystem>
#include <cstdint>
#include <vector>

namespace flowdaw {
struct AudioBuffer {
    int sampleRate = 44100;
    int channels = 2;
    std::vector<float> interleaved;
    SampleIndex frames() const { return channels > 0 ? static_cast<SampleIndex>(interleaved.size() / static_cast<std::size_t>(channels)) : 0; }
};

enum class WavEncoding { Float32, PCM24, PCM16 };
enum class DitherMode { None, TPDF };

struct WavWriteOptions {
    WavEncoding encoding=WavEncoding::Float32;
    DitherMode dither=DitherMode::None;
    std::uint32_t ditherSeed=0x464c4f57u;
};

class WavFile {
public:
    static AudioBuffer read(const std::filesystem::path& path);
    static void write(const std::filesystem::path& path,const AudioBuffer& audio,const WavWriteOptions& options={});
    static void writeFloat32(const std::filesystem::path& path,const AudioBuffer& audio);
};
}
