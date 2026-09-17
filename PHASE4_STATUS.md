# Phase 4 — Recording / Automation / Advanced Mixer

Status: **DONE**.

## Recording

- PortAudio bootstrap attempts mono input + stereo output, with output-only fallback when no default input exists.
- Recording storage is allocated before recording starts; the realtime callback performs bounded writes and atomic coordination only.
- Input monitoring is available on armed audio tracks.
- Recorded audio is written to WAV and registered as a normal `SampleAsset`.
- Tracks keep non-destructive `RecordingTake` entries with musical start/length.
- `activeTakeId` provides basic whole-take comp selection without deleting source takes.

## Automation

- Persistent automation lanes store musical points and use deterministic linear interpolation.
- Duplicate control-point ticks are normalized.
- Supported render targets: Track Volume/Pan, Bus Volume/Pan, Send Gain and Master Volume.
- Automation is evaluated in realtime/device and offline render paths.
- Studio can write Track Volume/Pan points at the current playhead and display the selected volume curve.

## Mixer / routing

- Track Volume/Pan/Mute/Solo.
- Persistent buses with their own Volume/Pan/Mute/Solo and gain inserts.
- Track output routing to Master or a selected Bus.
- Persistent sends with destination, gain, enable and pre/post-fader state.
- Mixer Studio view exposes Arm, Monitor, recording, take switching, routing, sends and bus controls.

## Export

- Offline master WAV export with configurable tail allowance.
- Per-track WAV stem export.
- Arrangement end calculation includes audio clips, Pattern placements and recording takes.

## Persistence / verification

- Project format **v9** persists recording, comp selection, buses, sends and automation while retaining backward loading through v1.
- Dedicated Phase 4 tests cover callback-path capture, monitoring, active takes, automation interpolation/rendering, buses, sends, migration and export.
- Legacy Phase 0–3 integration suites continue to run against the v9 model.
- GitHub Actions builds the full Studio executable and runs all core tests plus the stretch benchmark.

## Scope note

The current comping workflow selects a complete active take non-destructively. Region-based lane comping, punch ranges and advanced vocal editing can be layered onto the v9 take model later without changing recorded source audio.
