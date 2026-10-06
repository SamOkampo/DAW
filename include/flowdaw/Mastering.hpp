#pragma once
#include "flowdaw/Wav.hpp"
#include <cstdint>

namespace flowdaw {

struct LoudnessReport {
    double integratedLufs=-120.0;
    double momentaryMaxLufs=-120.0;
    double shortTermMaxLufs=-120.0;
    double loudnessRangeLu=0.0;
    double samplePeakDbfs=-120.0;
    double truePeakDbtp=-120.0;
};

struct MasteringTarget {
    double integratedLufs=-14.0;
    double truePeakCeilingDbtp=-1.0;
};

enum class DitherMode { none, tpdf };

struct MasteringExportOptions {
    int bitDepth=24;                 // 16, 24 or 32-float
    DitherMode dither=DitherMode::tpdf;
    bool normalizeLoudness=false;
    MasteringTarget target{};
    std::uint32_t ditherSeed=0x464c4f57u;
};

LoudnessReport analyzeLoudness(const AudioBuffer& audio);
void applyMasteringTarget(AudioBuffer& audio,const MasteringTarget& target);
double linearToDb(double value) noexcept;

} // namespace flowdaw
