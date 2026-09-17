# FLOWDAW audio engine

## Realtime contract

The shipping architecture remains C++20 with a JUCE 9.x desktop/device adapter. UI state, file I/O, analysis, project mutation and allocation-heavy work never belong in the realtime callback.

The control thread compiles project state into immutable render graphs. The audio callback reads the published graph, mixes timeline/pattern/take events and never mutates the Project.

## Musical scheduling

- Internal resolution: 960 PPQ.
- Pattern, MIDI, Chop and take timing derives from project BPM and ticks.
- Swing delays off subdivisions.
- Humanize and Probability are deterministic per event/repeat.
- Mixer automation stays in ticks until graph evaluation so BPM changes preserve musical placement.

## Smart sampling / instruments

BPM/transient analysis and WSOLA processing occur outside the callback. Match BPM creates derived buffers without modifying source audio. Native MIDI instruments are rendered into graph-owned buffers on publish, not synthesized by allocating inside the realtime callback.

## Preview

Chop/Piano preview uses a fixed SPSC command ring and 16 preallocated callback voices. The callback takes no preview mutex and performs no preview heap allocation. Control-side shared ownership keeps referenced buffers alive.

## Phase 4 audio recording

The bootstrap PortAudio adapter first attempts a mono-input/stereo-output stream and automatically falls back to output-only when no default input is available.

Before REC begins, the control thread allocates a bounded mono recording buffer. During recording the callback performs only bounded sample copies and atomic state updates. `finishRecording()` stops new writes, waits for any in-flight writer to leave the callback section, then returns a normal `AudioBuffer` to the control thread. WAV writing and project/take creation occur outside realtime.

Input monitoring is mixed into stereo output in the callback when enabled. CI feeds deterministic mono blocks into the same `process()` path used by a real device, so capture and monitoring remain testable without a physical microphone.

## Phase 4 mixer graph

Every published event is routed through a track channel and optionally a destination bus. Tracks can also create parallel sends to buses. Current graph state includes:

- Track Volume/Pan/Mute/Solo and gain inserts.
- Master or Bus output routing.
- Bus Volume/Pan/Mute/Solo and gain inserts.
- Send gain plus persisted pre/post-fader state.
- Master gain and gain inserts.
- Automation indexes for Track Volume/Pan, Bus Volume/Pan, Send Gain and Master Volume.

Automation values are linearly evaluated at the musical tick corresponding to each rendered output sample. The same graph logic is used for device playback and offline rendering/export.

## Offline export

Master WAV and per-track stem export publish an immutable project graph and render it offline. Arrangement duration includes clips, pattern placements and recording takes; an optional tail allowance preserves delay/release tails.

A device-block test exercises the callback mixer path while dedicated Phase 4 tests cover recording, monitoring, takes, automation, buses, sends and export.
