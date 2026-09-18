# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW in active development. Its core principle is simple: remove technical friction without removing creative control.

## Current state

The repository contains a runnable C++20 DAW foundation plus completed Step Sequencer/Groove Engine, Smart Sampling/Chop Mode, Piano Roll/MIDI/Native Instruments, Recording/Automation/Advanced Mixer, Assist/Plugins/Advanced Workflow, Production Platform/Reliability, and JUCE Runtime Foundation/Real VST3 Host phases.

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
- Persistent non-destructive recording takes with active-take comp selection.
- Advanced mixer routing, buses, sends, automation, master export and stems.
- Project format v10 persists Smart Sampling, MIDI/instrument, recording, mixer-routing, automation and plugin-rack state while loading v1-v9.
- Built-in FLOW Gain, FLOW Soft Clip and FLOW Width processors plus an SDK-neutral external-plugin backend contract.
- Context-aware production assistant, Project Health and Undoable advanced-workflow commands.
- Machine-local application settings for preferred sample rate, buffer size, input/output names, plugin roots and autosave policy.
- Crash-recovery infrastructure using a dirty-session marker plus versioned `autosave.flow` snapshot without changing the portable `.flow` schema.
- Persistent plugin quarantine registry with failure threshold and explicit reset.
- `flowdaw-doctor` command-line utility for runtime status, recovery export and plugin-quarantine maintenance.
- Install rules plus CPack release packaging; CI smoke-tests installed binaries and the generated package.
- Pinned optional JUCE 9.0.2 runtime with a real `flowdaw-juce` desktop target, JUCE device management and external-plugin backend registration.
- Real VST3 discovery, instantiation, audio processing and opaque-state roundtrip validated end-to-end with a deterministic JUCE-built VST3 fixture in CI.
- The X11 Studio remains available while editing views migrate; the JUCE runtime foundation does not yet imply full Studio feature parity.

**Phase 0 through Phase 7 are complete. Phase 8 — Realtime Plugin Graph / PDC / JUCE Studio Migration — is now in progress.**

Phase 8 is responsible for placing external VST3 processors inside the actual realtime track/bus/master graph with callback-safe lifecycle management and plugin-delay compensation, then moving the remaining editing surfaces from the X11 bootstrap to JUCE. Until that integration lands, the Phase 7 host proves real plugin execution through the host contract and integration fixture, but does not claim that arbitrary external plugins already execute on every realtime mixer route.

## Build

Core/X11 bootstrap:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/flowdaw
./build/flowdaw-doctor status
```

JUCE runtime (fetches the pinned JUCE 9.0.2 source when requested):

```bash
cmake -S . -B build-juce -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFLOWDAW_BUILD_APP=OFF \
  -DFLOWDAW_BUILD_TESTS=ON \
  -DFLOWDAW_ENABLE_JUCE_RUNTIME=ON \
  -DFLOWDAW_FETCH_JUCE=ON
cmake --build build-juce --parallel 2 --target flowdaw-juce flowdaw_phase7_juce_tests
ctest --test-dir build-juce -R flowdaw_phase7_juce_tests --output-on-failure
```

Open a WAV or project with the X11 bootstrap:

```bash
./build/flowdaw /path/to/sample.wav
./build/flowdaw projects/StepSequencer_90BPM.flow
```

Open the Piano Roll with `P`. Open the mixer with `M`; `F9` toggles audio recording when an input device is available. Open Phase 5 with `I` to access Assist, Project Health, plugin discovery/racks and advanced workflow commands.

Recovery/diagnostics:

```bash
flowdaw-doctor status
flowdaw-doctor recover recovered.flow
flowdaw-doctor clear-recovery
flowdaw-doctor clear-quarantine <plugin-id>
```

Package locally:

```bash
cmake --build build --parallel
cmake --install build --prefix ./install
cd build && cpack -G TGZ
```

Run the stretch benchmark:

```bash
cmake --build build --target flowdaw_stretch_benchmark
./build/flowdaw_stretch_benchmark
```

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md`, `ROADMAP.md`, `PHASE1_STATUS.md` through `PHASE7_STATUS.md`, and `docs/TIME_STRETCH_EVALUATION.md`.
