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
struct SampleSlice {
    Id id=nextId();
    std::string name="Slice";
    SampleIndex startFrame=0;
    SampleIndex endFrame=0;
    float gain=1.0f;
    float pan=0.0f;
    int chokeGroup=0;
};
struct SampleAsset {
    Id id=nextId();
    std::filesystem::path path;
    std::string name;
    std::string nativeKey;
    double detectedBpm=0.0;
    float bpmConfidence=0.0f;
    Id sourceSampleId=0;
    double timeRatio=1.0;
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
    Tick recordedTick=0;
    Tick tick=0;
    Id sampleId=0;
    Id sliceId=0;
    float recordedVelocity=1.0f;
    float velocity=1.0f;
    float pan=0.0f;
};
struct MidiNote {
    Id id=nextId();
    Tick startTick=0;
    Tick lengthTicks=kPPQ/2;
    int pitch=60;
    float velocity=0.9f;
};
struct InstrumentState {
    bool enabled=false;
    std::string type="flow_keys";
    float gain=0.8f;
    float pan=0.0f;
    float attackMs=5.0f;
    float releaseMs=120.0f;
    float tone=0.5f;
    float drive=0.0f;
    float delayMix=0.0f;
    Tick delayTicks=kPPQ/2;
};
struct Pattern {
    Id id=nextId();
    std::string name="Pattern 1";
    int stepCount=16;
    int stepsPerBeat=4;
    float swing=0.0f;
    float humanize=0.0f;
    std::vector<DrumLane> lanes;
    std::vector<ChopEvent> chopEvents;
    Tick chopQuantizeGridTicks=kPPQ/4;
    float chopQuantizeStrength=0.0f;
    float chopHumanize=0.0f;

    // Phase 3 MIDI/Piano Roll state. MIDI remains musical data; the native
    // instrument is rendered from these notes on publish and never baked into the project.
    std::vector<MidiNote> midiNotes;
    InstrumentState instrument;
    int scaleRoot=0;                    // pitch class, C=0
    std::string scaleType="minor";     // minor/major/minor_pentatonic/major_pentatonic
    Tick midiGridTicks=kPPQ/4;          // default 1/16 grid
    Tick midiDefaultLengthTicks=kPPQ/2; // default 1/8 note

    Tick lengthTicks() const { return static_cast<Tick>(stepCount)*kPPQ/stepsPerBeat; }
};
struct PatternPlacement { Id id=nextId(); Id patternId=0; Tick startTick=0; int repeats=1; };
struct Track { Id id=nextId(); std::string name="Audio 1"; MixerChannel mixer; std::vector<Clip> clips; std::vector<PatternPlacement> patternClips; };
struct TransportState { double bpm=90.0; Tick playheadTick=0; bool playing=false; };
struct MasterMixer { float volume=1.0f; std::vector<Effect> effects; };
struct Project {
    int formatVersion=8;
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
