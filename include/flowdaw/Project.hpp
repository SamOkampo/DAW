#pragma once
#include "flowdaw/Types.hpp"
#include "flowdaw/Wav.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw {
struct Effect { Id id=nextId(); std::string type="gain"; bool enabled=true; float value=1.0f; };
struct MixerChannel { float volume=1.0f; float pan=0.0f; bool mute=false; bool solo=false; std::vector<Effect> effects; };
struct SampleSlice { Id id=nextId(); std::string name="Slice"; SampleIndex startFrame=0; SampleIndex endFrame=0; };
struct SampleAsset {
    Id id=nextId();
    std::filesystem::path path;
    std::string name;
    std::string nativeKey;
    double detectedBpm=0.0;
    float bpmConfidence=0.0f;
    Id sourceSampleId=0;        // 0 = original. Non-zero = derived from another asset.
    double timeRatio=1.0;      // output duration / source duration for derived assets.
    std::vector<SampleSlice> slices;
    std::shared_ptr<AudioBuffer> audio;
};
struct Clip { Id id=nextId(); Id sampleId=0; Tick startTick=0; Tick lengthTicks=0; SampleIndex sourceStart=0; SampleIndex sourceLength=0; float gain=1.0f; bool loop=false; };
struct StepEvent { bool active=false; float velocity=1.0f; float probability=1.0f; int microTicks=0; };
struct DrumLane {
    std::string name;
    Id sampleId=0;
    float volume=1.0f;
    float pan=0.0f;
    bool mute=false;
    bool solo=false;
    std::vector<StepEvent> steps;
};
struct ChopEvent {
    Id id=nextId();
    Tick recordedTick=0;       // Exact timing captured from the performance.
    Tick tick=0;               // Edited/rendered timing; derived from recordedTick.
    Id sampleId=0;
    Id sliceId=0;
    float recordedVelocity=1.0f;
    float velocity=1.0f;       // Edited/rendered velocity; derived from recordedVelocity.
    float pan=0.0f;
};
struct Pattern {
    Id id=nextId();
    std::string name="Pattern 1";
    int stepCount=16;
    int stepsPerBeat=4;
    float swing=0.0f;      // 0..1; delays every second subdivision.
    float humanize=0.0f;   // 0..1; deterministic timing/velocity variation for drum lanes.
    std::vector<DrumLane> lanes;
    std::vector<ChopEvent> chopEvents;

    // Non-destructive Chop performance editing. applyChopEditing() derives each
    // event's tick/velocity from its recordedTick/recordedVelocity every time,
    // so changing these controls never accumulates timing error.
    Tick chopQuantizeGridTicks=kPPQ/4; // 1/16 note at 960 PPQ.
    float chopQuantizeStrength=0.0f;   // 0 = original performance, 1 = full snap.
    float chopHumanize=0.0f;           // deterministic timing/velocity variation.

    Tick lengthTicks() const { return static_cast<Tick>(stepCount)*kPPQ/stepsPerBeat; }
};
struct PatternPlacement { Id id=nextId(); Id patternId=0; Tick startTick=0; int repeats=1; };
struct Track { Id id=nextId(); std::string name="Audio 1"; MixerChannel mixer; std::vector<Clip> clips; std::vector<PatternPlacement> patternClips; };
struct TransportState { double bpm=90.0; Tick playheadTick=0; bool playing=false; };
struct MasterMixer { float volume=1.0f; std::vector<Effect> effects; };
struct Project {
    int formatVersion=6;
    std::string name="Untitled";
    int sampleRate=48000;
    TransportState transport;
    MasterMixer master;
    std::vector<SampleAsset> samples;
    std::vector<Track> tracks;
    std::vector<Pattern> patterns;

    SampleAsset* findSample(Id id); const SampleAsset* findSample(Id id) const;
    Track* findTrack(Id id); Pattern* findPattern(Id id); const Pattern* findPattern(Id id) const;
};
}
