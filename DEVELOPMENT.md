# Development

## Quality gate for every feature

A feature is not complete until:

`visible UI -> user action -> Project/command state -> render graph update -> audible/visible result -> Undo -> save/open roundtrip`

No decorative buttons are accepted in production Studio screens.

## Build and test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

Useful sanitizers for local developer builds:

```bash
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

Realtime testing additionally needs an actual audio device and latency/xrun instrumentation. The current CI/container cannot validate physical-device latency because it has no default ALSA/CoreAudio/WASAPI device.

## Phase 0 bootstrap UI controls

- PLAY / PAUSE / STOP buttons.
- BPM `-` / `+`.
- IMPORT WAV: type an absolute/relative WAV path and press Enter.
- SAVE: writes the current project to `Untitled.flow` unless another project was opened.
- OPEN: type a `.flow` path and press Enter.
- Drag the audio clip horizontally; it snaps to 1/16-note increments.
- Space: play/pause.
- Ctrl+S: save.
- Ctrl+Z / Ctrl+Shift+Z: undo/redo.
