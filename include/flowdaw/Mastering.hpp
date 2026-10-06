#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
#include <limits>
#include <memory>

namespace flowdaw {

class PluginHost;

struct MasteringMetrics {
    double integratedLufs=-std::numeric_limits<double>::infinity();
    double momentaryMaxLufs=-std::numeric_limits<double>::infinity();
    double shortTermMaxLufs=-std::numeric_limits<double>::infinity();
    double loudnessRangeLu=0.0;
    double maxTruePeakDbTP=-std::numeric_limits<double>::infinity();
    double maxSamplePeakDbFS=-std::numeric_limits<double>::infinity();
};

struct LoudnessTarget {
    double targetLufs=-23.0;
    double maxTruePeakDbTP=-1.0;
    double toleranceLu=0.2;
};

struct LoudnessNormalizationResult {
    MasteringMetrics before;
    MasteringMetrics after;
    double requestedGainDb=0.0;
    double appliedGainDb=0.0;
    bool limitedByTruePeak=false;
    bool targetWithinTolerance=false;
};

enum class MasterBitDepth { Float32, PCM24, PCM16 };
enum class DitherMode { None, TPDF };

struct MasterExportOptions {
    MasterBitDepth bitDepth=MasterBitDepth::PCM24;
    DitherMode dither=DitherMode::TPDF;
    bool normalizeLoudness=false;
    LoudnessTarget target{};
};

struct MasterExportReport {
    MasteringMetrics rendered;
    MasteringMetrics delivered;
    double appliedGainDb=0.0;
    bool normalized=false;
    bool limitedByTruePeak=false;
    bool targetWithinTolerance=false;
    MasterBitDepth bitDepth=MasterBitDepth::PCM24;
    DitherMode dither=DitherMode::TPDF;
};

MasteringMetrics analyzeMastering(const AudioBuffer& audio);
LoudnessNormalizationResult normalizeLoudness(AudioBuffer& audio,const LoudnessTarget& target);
MasterExportReport exportMasteredWav(
    const Project& project,
    const std::filesystem::path& path,
    const MasterExportOptions& options={},
    double tailSeconds=2.0,
    std::shared_ptr<PluginHost> pluginHost={});

} // namespace flowdaw
