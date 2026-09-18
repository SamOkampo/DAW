#pragma once
#include "flowdaw/Types.hpp"
#include "flowdaw/Wav.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw {
struct Effect { Id id=nextId(); std::string type="gain"; bool enabled=true; float value=1.0f; };

// Phase 5 plugin state is backend-agnostic. The core persists format/identifier,
// normalized parameters and an opaque backend state chunk without depending on an SDK.
struct PluginParameter {
    std::string id;
    float value=0.0f;
};
struct PluginInstance {
    Id id=nextId();
    std::string format="builtin"; // builtin, vst3, au
    std::string identifier="flow.gain";
    std::string name="FLOW Gain";
    bool enabled=true;
    bool bypass=false;
    float wet=1.0f;
    std::string opaqueState;
    std::vector<PluginParameter> parameters;
};
struct MixerChannel {
    float volume=1.0f;
    float pan=0.0f;
    bool mute=false;
    bool solo=false;
    std::vector<Effect> effects;
    std::vector<PluginInstance> plugins;
};
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
    std::vector<MidiNote> midiNotes;
    InstrumentState instrument;
    int scaleRoot=0;
    std::string scaleType="minor";
    Tick midiGridTicks=kPPQ/4;
    Tick midiDefaultLengthTicks=kPPQ/2;
    Tick lengthTicks() const { return static_cast<Tick>(stepCount)*kPPQ/stepsPerBeat; }
};
struct PatternPlacement { Id id=nextId(); Id patternId=0; Tick startTick=0; int repeats=1; };

struct RecordingTake {
    Id id=nextId();
    std::string name="Take";
    Id sampleId=0;
    Tick startTick=0;
    Tick lengthTicks=0;
};
struct MixerSend {
    Id id=nextId();
    Id busId=0;
    float gain=0.0f;
    bool enabled=true;
    bool preFader=false;
};
struct Bus {
    Id id=nextId();
    std::string name="Bus";
    MixerChannel mixer;
};
struct AutomationPoint {
    Tick tick=0;
    float value=1.0f;
};
struct AutomationLane {
    Id id=nextId();
    std::string target="track.volume";
    Id targetId=0;
    Id subTargetId=0;
    std::vector<AutomationPoint> points;
};
struct Track {
    Id id=nextId();
    std::string name="Audio 1";
    MixerChannel mixer;
    // Phase 8: external instrument is a source slot, separate from mixer inserts.
    // Legacy/native Pattern::instrument remains available when this is disabled.
    bool externalInstrumentEnabled=false;
    PluginInstance externalInstrument;
    std::vector<Clip> clips;
    std::vector<PatternPlacement> patternClips;
    bool armed=false;
    bool inputMonitor=false;
    Id outputBusId=0;
    std::vector<MixerSend> sends;
    std::vector<RecordingTake> takes;
    Id activeTakeId=0;
};
struct TransportState { double bpm=90.0; Tick playheadTick=0; bool playing=false; };
struct MasterMixer {
    float volume=1.0f;
    std::vector<Effect> effects;
    std::vector<PluginInstance> plugins;
};
struct Project {
    int formatVersion=11;
    std::string name="Untitled";
    int sampleRate=48000;
    TransportState transport;
    MasterMixer master;
    std::vector<SampleAsset> samples;
    std::vector<Track> tracks;
    std::vector<Pattern> patterns;
    std::vector<Bus> buses;
    std::vector<AutomationLane> automation;

    SampleAsset* findSample(Id id); const SampleAsset* findSample(Id id) const;
    Track* findTrack(Id id); Pattern* findPattern(Id id); const Pattern* findPattern(Id id) const;
    Bus* findBus(Id id); const Bus* findBus(Id id) const;
};
}
