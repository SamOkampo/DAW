# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW in active development. Its core principle is simple: remove technical friction without removing creative control.

## Current state

The repository contains a runnable C++20 DAW foundation plus completed Step Sequencer/Groove Engine, Smart Sampling/Chop Mode, Piano Roll/MIDI/Native Instruments, Recording/Automation/Advanced Mixer, and Assist/Plugins/Advanced Workflow phases.

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
- Project format v10 persists Smart Sampling, MIDI/instrument, recording, mixer-routing, automation and plugin-rack state while loading v1-v9.
- Persistent plugin racks on Master, Tracks and Buses with bypass, wet mix, parameters and opaque state.
- Built-in FLOW Gain, FLOW Soft Clip and FLOW Width processors; native master processing runs in realtime/offline rendering.
- SDK-neutral external plugin backend contract plus VST3/AU bundle discovery that does not execute third-party binaries during scanning.
- Context-aware production assistant for headroom, gain staging, groove, buses, takes and sample-tempo checks.
- Assistant edits are explicit and Undoable; informational advice never mutates the project automatically.
- Project Health validation for broken routing/references, duplicate IDs and suspicious project state.
- Advanced workflow commands for common production actions.
- Safer project saving using temporary replacement plus `.bak` backup of the previous project.
- GitHub Actions validates nine core/test suites, the stretch benchmark and the complete Phase 0–5 Studio executable.

**Phase 0 through Phase 5 are complete. The next milestone is production desktop hardening: JUCE 9.x UI/device integration, a real VST3 backend, AU hosting on macOS, plugin-editor hosting/sandboxing, packaging, crash recovery and release-grade profiling.**

The current Linux bootstrap UI uses X11 because the development environment does not ship JUCE headers. The intended shipping backend remains **C++20 + JUCE 9.x** for Windows/macOS. The musical/audio core is toolkit-independent so the bootstrap shell can be replaced without rewriting the project model or scheduler.

External plugin support in the bootstrap is deliberately honest: FLOWDAW can discover `.vst3` and `.component` bundles, persist them as plugin slots and expose a tested `IExternalPluginBackend` contract, but it does **not** execute third-party VST3/AU binaries unless a real backend is registered. Built-in FLOW plugins do process audio now.

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

Open the Piano Roll with `P`. Open the Phase 4 mixer with `M`; `F9` toggles audio recording when an input device is available. Open Phase 5 with `I` to access Assist, Project Health, plugin discovery/racks and advanced workflow commands. Assistant changes only run after explicit Apply and are committed to Undo.

Run the stretch benchmark:

```bash
cmake --build build --target flowdaw_stretch_benchmark
./build/flowdaw_stretch_benchmark
```

CI does not have a physical microphone or third-party commercial plugins. Recording correctness is tested by feeding deterministic mono input through the exact callback capture/monitor path; external plugin hosting is tested through the same backend contract with a deterministic fake backend. The complete Studio executable is compiled separately under the Linux bootstrap path.

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md`, `ROADMAP.md`, `PHASE1_STATUS.md`, `PHASE2_STATUS.md`, `PHASE3_STATUS.md`, `PHASE4_STATUS.md`, `PHASE5_STATUS.md` and `docs/TIME_STRETCH_EVALUATION.md`.
