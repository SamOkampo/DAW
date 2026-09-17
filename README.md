# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW in active development. The product goal is to remove technical friction without removing creative control.

## Current state

The repository now contains a runnable C++20 DAW foundation plus the first real Step Sequencer/Groove Engine layer.

Implemented and tested:

- C++20 core separated from UI/device backends.
- 960 PPQ musical clock and deterministic Bars/Beats/Ticks ↔ seconds ↔ samples conversion.
- Project model: Project, Track, Clip, Pattern, SampleAsset, Transport, MixerChannel and Effect.
- WAV import for mono/stereo PCM 16/24/32-bit and float32.
- Non-destructive clip placement; original source audio is never modified.
- Timeline, waveform rendering and draggable audio clip in the bootstrap desktop UI.
- Play / Pause / Stop and BPM editing.
- Immutable render graph published from the control thread; the audio callback performs no project mutation.
- Stereo mixing, volume/pan and Phase-0 linear interpolation sample-rate conversion.
- Versioned project save/open with backward loading for Phase-0/early-Phase-1 project files.
- Undo/Redo for clip movement, imports, BPM, steps, groove values and per-step edits.
- 16/32/64-step Kick/Snare/Hat sequencer using original FLOWDAW-generated drum synthesis.
- Per-step Velocity, Probability and Microtiming editors.
- Pattern Swing and deterministic Humanize that affect actual event scheduling.
- Lane Mute/Solo.
- Visible, draggable Pattern placement in Arrangement with grid snap and repeats.
- Mixer/master Effect objects persisted in project format v3.
- Automated tests proving timing conversion, WAV roundtrip, waveform generation, serialization, effects persistence, undo/redo, audible render, step sequencing, long-pattern scheduling, arrangement offsets, probability, swing, microtiming, humanization and lane mute behavior.
- GitHub Actions core CI.

The bootstrap GUI uses X11 because the execution environment does not ship JUCE headers. The production desktop backend remains **JUCE 9.x + C++20**; the core is toolkit-independent so this shell can be replaced without rewriting the musical model or audio engine.

## Build — bootstrap Linux shell

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
./build/flowdaw
```

To open a project or WAV directly:

```bash
./build/flowdaw projects/StepSequencer_90BPM.flow
./build/flowdaw /path/to/sample.wav
```

The current development container has no physical/default audio device. The GUI and offline renderer are verified here; the temporary realtime device bridge uses PortAudio when a device is available.

## Architecture direction

Production builds should pin JUCE, use its Windows/macOS audio-device layer, and retain the independent domain/audio core in `include/flowdaw` and `src/`. The X11/PortAudio code is a bootstrap adapter, not the intended shipping UI.

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md`, `DEVELOPMENT.md`, `ROADMAP.md`, and `PHASE1_STATUS.md`.
