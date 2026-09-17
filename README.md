# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW in active development. Its core principle is simple: remove technical friction without removing creative control.

## Current state

The repository contains a runnable C++20 DAW foundation plus completed Step Sequencer/Groove Engine, Smart Sampling/Chop Mode, Piano Roll/MIDI/Native Instruments, and Recording/Automation/Advanced Mixer phases.

### Working and tested

- C++20 domain/audio core separated from UI and device backends.
- 960 PPQ musical clock and deterministic Bars/Beats/Ticks ↔ seconds ↔ samples conversion.
- Versioned `.flow` project model with non-destructive source references and backward loading through v1.
- WAV import, waveform display, draggable audio clips and Pattern blocks.
- Play / Pause / Stop, BPM and Undo/Redo.
- 16/32/64-step drums with native sounds, Velocity, Probability, Microtiming, Swing/Humanize and lane mixer controls.
- Smart Sampling with BPM/transient analysis, beat/downbeat grid, Equal/Auto/Beat/Bar Chop, editable slices, multi-bank pads, Gain/Pan/Choke and REC CHOPS.
- WSOLA Match BPM baseline with original-source preservation, quality tests and performance benchmark.
- Piano Roll with note creation, selection, delete, drag, pitch movement, resize, velocity/length editing, 1/8–1/32 grids and scale highlighting.
- Native melodic instruments: FLOW Keys, FLOW 808, FLOW Bass and FLOW Lead, with envelope/tone, Drive and tempo-synced Delay.
- MIDI notes render through the same Arrangement scheduler used by drums and chops.
- Audio input capture using a preallocated realtime-safe recording buffer.
- Input monitoring plus automatic output-only fallback when a default microphone is unavailable.
- Persistent non-destructive recording takes with active-take comp selection.
- Advanced mixer routing: track fader/pan/mute/solo, Master or Bus output, buses and pre/post-fader-capable sends.
- Persistent linear automation for Track Volume/Pan, Bus Volume/Pan, Send Gain and Master Volume.
- Mixer/REC Studio view with Arm, Monitor, REC AUDIO, take switching, routing, sends, buses and automation-point writing.
- Master WAV export plus per-track WAV stem export.
- Project format v9 persists Smart Sampling, MIDI/instrument, recording, mixer-routing and automation state while loading v1-v8.
- GitHub Actions validates eight core/test suites, the stretch benchmark and the complete Studio executable.

**Phase 0, Phase 1, Phase 2, Phase 3 and Phase 4 are complete. Phase 5 (Assist / Plugins / Advanced Workflow) is next.**

The current Linux bootstrap UI uses X11 because the development environment does not ship JUCE headers. The intended shipping backend remains **C++20 + JUCE 9.x** for Windows/macOS. The musical/audio core is toolkit-independent so the bootstrap shell can be replaced without rewriting the project model or scheduler.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/flowdaw
```

Open a WAV or project directly:

```bash
./build/flowdaw /path/to/sample.wav
./build/flowdaw projects/StepSequencer_90BPM.flow
```

Open the Piano Roll with `P`. Open the Phase 4 mixer with `M`; `F9` toggles audio recording when an input device is available. In the mixer, Arm selects a recording destination, Monitor enables input monitoring, take arrows change the non-destructive comp, automation buttons write the selected track's current value at the playhead, and Export WAV / Stems produce offline renders.

Run the stretch benchmark:

```bash
cmake --build build --target flowdaw_stretch_benchmark
./build/flowdaw_stretch_benchmark
```

CI does not have a physical microphone, so recording correctness is tested by feeding deterministic mono input through the exact callback capture/monitor path. The complete Studio executable is compiled separately under the Linux bootstrap path.

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md`, `ROADMAP.md`, `PHASE1_STATUS.md`, `PHASE2_STATUS.md`, `PHASE3_STATUS.md`, `PHASE4_STATUS.md` and `docs/TIME_STRETCH_EVALUATION.md`.
