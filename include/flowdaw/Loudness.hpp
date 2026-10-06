#pragma once
#include "flowdaw/Wav.hpp"
#include <limits>

namespace flowdaw {

struct LoudnessReport {
    double integratedLufs=-std::numeric_limits<double>::infinity();
    double momentaryMaxLufs=-std::numeric_limits<double>::infinity();
    double shortTermMaxLufs=-std::numeric_limits<double>::infinity();
    double loudnessRangeLu=0.0;
    double samplePeakLinear=0.0;
    double samplePeakDbfs=-std::numeric_limits<double>::infinity();
    double truePeakLinear=0.0;
    double truePeakDbtp=-std::numeric_limits<double>::infinity();
    bool silence=true;
};

class MasteringAnalyzer {
public:
    // Offline/control-thread analysis aligned with ITU-R BS.1770 loudness
    // gating and 4x band-limited true-peak estimation. This implementation
    // is standards-aligned but is not an external conformance certification.
    static LoudnessReport analyze(const AudioBuffer& audio);
    static double truePeakLinear(const AudioBuffer& audio);
};

} // namespace flowdaw
