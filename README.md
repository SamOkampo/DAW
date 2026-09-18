# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW built around one principle: remove technical friction without removing creative control.

## Current state

**Phase 0 through Phase 8 are complete.**

The production desktop path is now the **JUCE 9.0.2 Studio**. The original X11 shell remains source-available only as an explicit legacy regression/bootstrap target.

### Working and tested

- C++20 domain/audio core separated from UI/device/plugin backends.
- 960 PPQ musical clock and versioned non-destructive `.flow` projects with backward loading through v1.
- Arrangement editing for audio and Pattern blocks, transport, BPM and Undo/Redo.
- 16/32/64-step drum sequencer with native FLOW drums, Velocity, Probability, Microtiming, lane Volume/Pan/Mute/Solo, Swing/Humanize and Straight/Boom Bap/Loose presets.
- Smart Sampling with WAV import, BPM/beat-grid analysis, Equal/Auto/Beat/Bar Chop, editable slices, multi-bank pads, Rename/Gain/Pan/Choke, keyboard performance and non-destructive Match BPM.
- REC CHOPS with persistent performances, 1/8–1/32 grids, Quantize 0–100%, Humanize 0–100% and Reset Feel.
- Piano Roll with note add/move/resize/delete, velocity/length, 1/8–1/32 grid, root/scale guidance, keyboard preview and octave navigation.
- Native FLOW Keys, FLOW 808, FLOW Bass and FLOW Lead with gain, pan, envelope, tone, Drive and tempo-aware Delay.
- Realtime-safe audio recording, input monitoring, persistent takes and active-take selection.
- Mixer routing with buses, sends, pre/post-fader mode, Track/Bus/Master automation and route metering.
- Realtime Track/Bus/Master plugin graph with topology-aware plugin delay compensation.
- Callback-safe sample peak, RMS and 4x inter-sample true-peak estimates for Track/Bus/Master routes.
- Real VST3 execution through JUCE on Linux/Windows/macOS and AU runtime validation on macOS.
- Persistent Track/Master plugin racks with FLOW Gain/Soft Clip/Width, bypass, wet mix, native parameters and external-plugin editor state roundtrip.
- External instrument slots driven by Pattern MIDI before Track inserts.
- Realtime/offline routing parity, master WAV export and per-track stem export through the production PluginHost.
- Production Assistant, Project Health and explicit Undoable workflow commands.
- Machine-local audio/plugin settings, crash recovery/autosave infrastructure and plugin quarantine.
- `flowdaw-doctor` for runtime status, recovery and plugin-quarantine maintenance.
- New sessions start with a usable native beat plus a FLOW Keys melodic Pattern.
- Cross-platform CI validates Linux, Windows and macOS production builds; Windows creates a portable ZIP and macOS creates a DMG.

Project format is currently **v11**.

## Build the production Studio

JUCE 9.0.2 is pinned and fetched by default for the production build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel 2 --target flowdaw-juce flowdaw-doctor
ctest --test-dir build --output-on-failure
```

The installed JUCE application is named **FLOWDAW**.

Package the production build on Linux:

```bash
cmake --install build --prefix ./install
cd build
cpack -G TGZ
```

Windows ZIP and macOS DMG generation are validated in CI.

JUCE modules are distributed under JUCE's upstream licensing terms. Commercial distribution requires an applicable JUCE licence and compliance with its terms.

## Core-only development

The domain/audio core remains toolkit-independent. Because JUCE is enabled by default for production builds, disable it explicitly for a core-only build:

```bash
cmake -S . -B build-core -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFLOWDAW_BUILD_APP=OFF \
  -DFLOWDAW_BUILD_TESTS=ON \
  -DFLOWDAW_ENABLE_JUCE_RUNTIME=OFF \
  -DFLOWDAW_FETCH_JUCE=OFF

cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
./build-core/flowdaw-doctor status
```

## Legacy X11 regression shell

The X11 bootstrap is **not** the production Studio and is disabled by default. It remains available for regression/bootstrap coverage:

```bash
cmake -S . -B build-x11 -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DFLOWDAW_BUILD_APP=ON \
  -DFLOWDAW_BUILD_TESTS=OFF \
  -DFLOWDAW_ENABLE_JUCE_RUNTIME=OFF \
  -DFLOWDAW_FETCH_JUCE=OFF

cmake --build build-x11 --parallel --target flowdaw flowdaw-doctor
./build-x11/flowdaw
```

CI keeps this legacy target smoke-tested while JUCE remains the single production desktop path.

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
