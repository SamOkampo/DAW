#pragma once
#include "flowdaw/Wav.hpp"
#include <limits>

namespace flowdaw {

struct MasteringAnalysis {
    double integratedLufs=-std::numeric_limits<double>::infinity();
    double maxMomentaryLufs=-std::numeric_limits<double>::infinity();
    double maxShortTermLufs=-std::numeric_limits<double>::infinity();
    double loudnessRangeLu=0.0;
    double samplePeakDbfs=-std::numeric_limits<double>::infinity();
    double truePeakDbtp=-std::numeric_limits<double>::infinity();
    double durationSeconds=0.0;
};

struct MasteringTarget {
    double targetLufs=-23.0;
    double maxTruePeakDbtp=-1.0;
    double loudnessToleranceLu=0.5;
};

struct MasteringCompliance {
    bool loudnessInTolerance=false;
    bool truePeakWithinLimit=false;
    bool compliant=false;
    double loudnessErrorLu=0.0;
    double truePeakMarginDb=0.0;
};

MasteringAnalysis analyzeMasteringAudio(const AudioBuffer& audio);
MasteringCompliance evaluateMasteringCompliance(const MasteringAnalysis& analysis,const MasteringTarget& target={});

// Gain-only two-pass normalization. The requested loudness gain is limited so
// the measured true peak cannot exceed maxTruePeakDbtp. This function does not
// hide limiting/clipping behind the export path.
double normalizeMasteringGain(AudioBuffer& audio,const MasteringTarget& target,MasteringAnalysis* before=nullptr,MasteringAnalysis* after=nullptr);

} // namespace flowdaw
