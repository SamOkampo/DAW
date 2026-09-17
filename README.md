# FLOWDAW

FLOWDAW is an experimental desktop DAW core focused on Hip-Hop-first workflows: sampling, drums, groove and low-friction editing without removing creative control.

## Current status

Phase 0 foundation and the first Phase 1 Step Sequencer/Groove Engine are implemented.

### Working now

- C++20 domain/audio core.
- 960 PPQ musical clock.
- Transport state: play, pause, stop, BPM.
- Timeline with draggable audio clips.
- WAV import and waveform generation.
- Non-destructive project model.
- Save/open `.flow` projects.
- Undo/Redo command snapshots.
- Immutable realtime render graph publication.
- Offline audio render tests.
- 16-step drum sequencer.
- Native FLOWDAW Kick, Snare and Hat synthesis.
- Pattern placement in arrangement.
- Per-step Velocity, Probability and Microtiming.
- Pattern Swing and deterministic Humanize.
- Per-lane Mute and Solo.
- Sequencer state persistence.

The long-term product backend is C++20 + JUCE 9.x. The current Linux bootstrap shell uses X11 and the PortAudio runtime available in the development environment so the core can be executed and tested without coupling domain code to the temporary shell.

## Build core/tests

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFLOWDAW_BUILD_APP=OFF -DFLOWDAW_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

See `ARCHITECTURE.md`, `AUDIO_ENGINE.md`, `PROJECT_FORMAT.md` and `ROADMAP.md` for the design contract.
