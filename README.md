# FLOWDAW

FLOWDAW is a hip-hop-first desktop DAW in active development. Its core principle is simple: remove technical friction without removing creative control.

## Current state

The repository contains a runnable C++20 DAW foundation, a real Step Sequencer/Groove Engine, and the first Smart Sampling/Chop workflow.

### Working and tested

- C++20 domain/audio core separated from UI and device backends.
- 960 PPQ musical clock and deterministic Bars/Beats/Ticks ↔ seconds ↔ samples conversion.
- Versioned `.flow` project model with non-destructive sample references.
- WAV import, waveform display, draggable audio clips and draggable Pattern blocks.
- Play / Pause / Stop, BPM and Undo/Redo.
- 16/32/64-step drum patterns with paged editing.
- Native FLOWDAW drums: Kick, Snare, Closed Hat, Open Hat, Clap, Rim and Perc.
- Per-step Velocity, Probability and Microtiming.
- Real Swing and deterministic Humanize scheduling.
- Lane Volume, Pan, Mute and Solo.
- Straight / Boom Bap / Loose groove presets.
- BPM analysis and transient detection.
- Equal Chop and Auto Chop, stored as non-destructive sample slices.
- Pitch-preserving WSOLA Match BPM baseline.
- Derived Match-BPM assets regenerate from the original asset + stretch ratio.
- 16-pad Chop Mode mapped to `1 2 3 4 / Q W E R / A S D F / Z X C V`.
- Chop pads use a fixed, lock-free preview command queue and fixed realtime voices; preview works independently of the transport.
- Slice markers are shown directly on the waveform.
- GitHub Actions core CI.

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

The development container has no physical/default audio device. The exact callback path is still covered through deterministic device-block tests, while the GUI is validated under a virtual X display.

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md`, `ROADMAP.md`, `PHASE1_STATUS.md` and `PHASE2_STATUS.md`.
