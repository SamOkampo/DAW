# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW in active development. Its core principle is simple: remove technical friction without removing creative control.

## Current state

The repository contains a runnable C++20 DAW foundation plus completed Step Sequencer/Groove Engine, Smart Sampling/Chop Mode and Piano Roll/MIDI/Native Instruments phases.

### Working and tested

- C++20 domain/audio core separated from UI and device backends.
- 960 PPQ musical clock and deterministic Bars/Beats/Ticks ↔ seconds ↔ samples conversion.
- Versioned `.flow` project model with non-destructive sample references and backward loading through v1.
- WAV import, waveform display, draggable audio clips and draggable Pattern blocks.
- Play / Pause / Stop, BPM and Undo/Redo.
- 16/32/64-step drum patterns with paged editing.
- Native FLOWDAW drums: Kick, Snare, Closed Hat, Open Hat, Clap, Rim and Perc.
- Per-step Velocity, Probability and Microtiming.
- Real Swing and deterministic Humanize scheduling.
- Lane Volume, Pan, Mute and Solo.
- Straight / Boom Bap / Loose groove presets.
- BPM analysis, transient detection and confidence-scored beat/downbeat grid.
- Equal Chop, Auto Chop, Beat Chop and Bar Chop, stored as non-destructive sample slices.
- Beat/downbeat overlay on the waveform plus manual marker editing.
- Multi-bank Chop Mode mapped to `1 2 3 4 / Q W E R / A S D F / Z X C V`; `[` and `]` change banks.
- Editable slice names plus persistent Gain, Pan and Choke Group per pad.
- Chop preview uses a fixed SPSC command queue and fixed realtime voices; same-group choke works while transport is stopped.
- REC CHOPS captures pad performance as editable musical events in a dedicated CHOPS Arrangement lane.
- Reversible 0–100% Chop Quantize, deterministic Chop Humanize, 1/8 / 1/16 / 1/32 grids and Reset Feel.
- Pitch-preserving WSOLA Match BPM baseline over the tested 0.5x–2.0x range.
- Derived Match-BPM assets regenerate from the original asset + stretch ratio instead of overwriting source audio.
- Piano Roll with click-to-create, select, delete, note drag, pitch drag and right-edge resize.
- Persistent MIDI notes with tick position, duration, pitch and velocity.
- MIDI 1/8 / 1/16 / 1/32 grid, scale-root selection and highlighted Major / Minor / Pentatonic scales.
- Keyboard note preview and octave navigation.
- Native melodic instruments: FLOW Keys, FLOW 808, FLOW Bass and FLOW Lead.
- Native instrument envelope/tone state plus soft Drive and tempo-synced Delay.
- MIDI patterns render through the same Arrangement scheduler used by drums and chops.
- Project format v8 persists complete Smart Sampling + MIDI/instrument state while loading v1-v7.
- Automated stretch quality tests, realtime-factor benchmark and dedicated MIDI/instrument tests.
- GitHub Actions validates all seven core/test suites and builds the complete Studio executable.

**Phase 0, Phase 1, Phase 2 and Phase 3 are complete. Phase 4 (Recording / Automation / Advanced Mixer) is next.**

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

Open the Piano Roll with `P`. Inside the Piano Roll, click empty grid space to create notes, drag notes to move them, drag the right edge to resize, right-click a note to delete it, and use `A W S E D F T G Y H U J` for pitch preview.

Run the stretch benchmark:

```bash
cmake --build build --target flowdaw_stretch_benchmark
./build/flowdaw_stretch_benchmark
```

The development container has no physical/default audio device. The exact callback path is covered through deterministic device-block tests, while the GUI is compiled in CI and validated under the bootstrap Linux path.

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md`, `ROADMAP.md`, `PHASE1_STATUS.md`, `PHASE2_STATUS.md`, `PHASE3_STATUS.md` and `docs/TIME_STRETCH_EVALUATION.md`.
