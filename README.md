# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW built around one principle: remove technical friction without removing creative control.

## Current state

**Phase 0 through Phase 8 are complete.** The production desktop path is the JUCE Studio; the original X11 shell remains available only as an opt-in regression/bootstrap target.

### Working and tested

- C++20 domain/audio core separated from UI and device backends.
- 960 PPQ musical clock and versioned non-destructive `.flow` projects with backward loading through v1.
- Arrangement editing for audio and Pattern blocks, transport, BPM and Undo/Redo.
- 16/32/64-step drum sequencer with native FLOW drums, Velocity, Probability, Microtiming, lane mixer controls, Swing/Humanize and Straight/Boom Bap/Loose presets.
- Smart Sampling with WAV import, BPM/beat-grid analysis, Equal/Auto/Beat/Bar Chop, editable slices, pad banks, Rename/Gain/Pan/Choke, keyboard performance and non-destructive Match BPM.
- REC CHOPS with persistent performances, 1/8–1/32 grid, Quantize 0–100%, Humanize 0–100% and Reset Feel.
- Piano Roll with note creation/movement/resize/delete, velocity/length, scale guidance, keyboard preview and octave navigation.
- Native FLOW Keys, FLOW 808, FLOW Bass and FLOW Lead with envelope, tone, Drive and tempo-aware Delay.
- Realtime-safe input capture, input monitoring, persistent takes and active-take selection.
- Mixer routing with buses, sends, pre/post-fader mode, automation and route metering.
- Realtime track/bus/master plugin graph with topology-aware plugin delay compensation.
- Callback-safe sample peak, RMS and true-peak estimates for track, bus and master routes.
- Real VST3 execution through JUCE on Linux/Windows/macOS and AU host/runtime validation on macOS.
- Persistent Track/Master plugin racks with bypass, wet mix, FLOW Gain/Soft Clip/Width parameters and live external-plugin editor state.
- External instrument slots driven by Pattern MIDI before track inserts.
- Realtime/offline routing parity, master WAV export and per-track stem export through the production PluginHost.
- Context-aware Production Assistant, Project Health and explicit Undoable workflow commands.
- Machine-local audio/plugin settings, crash recovery, autosave infrastructure and plugin quarantine.
- `flowdaw-doctor` for runtime status, recovery and plugin-quarantine maintenance.
- New sessions start with a usable native drum beat plus a FLOW Keys melodic Pattern.
- Windows portable ZIP and macOS DMG packaging are validated in CI.

Project format is currently **v11**.

## Build the production JUCE Studio

JUCE 9.0.2 is pinned by the build. To fetch it automatically:

```bash
cmake -S . -B build-juce -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFLOWDAW_BUILD_APP=OFF \
  -DFLOWDAW_BUILD_TESTS=ON \
  -DFLOWDAW_ENABLE_JUCE_RUNTIME=ON \
  -DFLOWDAW_FETCH_JUCE=ON \
  -DFLOWDAW_BUILD_JUCE_APP=ON \
  -DFLOWDAW_BUILD_JUCE_TEST_PLUGIN=ON

cmake --build build-juce --parallel 2 --target \
  flowdaw-juce flowdaw_phase7_juce_tests flowdaw_phase8_juce_tests flowdaw-doctor

ctest --test-dir build-juce --output-on-failure
```

The installed application is named **FLOWDAW**. JUCE modules are available under JUCE's upstream licensing terms; commercial distributors must use an applicable JUCE licence and comply with its terms.

## Core-only development

A toolkit-independent build remains available for core tests, Doctor and benchmarks:

```bash
cmake -S . -B build-core -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFLOWDAW_BUILD_APP=OFF \
  -DFLOWDAW_BUILD_TESTS=ON

cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
./build-core/flowdaw-doctor status
```

## Legacy X11 regression shell

The old X11 Studio is no longer the default product UI. It is retained for regression/bootstrap coverage and must be enabled explicitly:

```bash
cmake -S . -B build-x11 -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFLOWDAW_BUILD_APP=ON \
  -DFLOWDAW_BUILD_TESTS=OFF

cmake --build build-x11 --parallel --target flowdaw flowdaw-doctor
./build-x11/flowdaw
```

CI continues to build and package this legacy target so regressions remain visible while JUCE is the production desktop path.

## Diagnostics and recovery

```bash
flowdaw-doctor status
flowdaw-doctor recover recovered.flow
flowdaw-doctor clear-recovery
flowdaw-doctor clear-quarantine <plugin-id>
```

## Stretch benchmark

```bash
cmake --build build-core --target flowdaw_stretch_benchmark
./build-core/flowdaw_stretch_benchmark
```

## Project documentation

See:

- `ARCHITECTURE.md`
- `AUDIO_ENGINE.md`
- `PROJECT_FORMAT.md`
- `ROADMAP.md`
- `PHASE1_STATUS.md` through `PHASE8_STATUS.md`
- `docs/TIME_STRETCH_EVALUATION.md`
