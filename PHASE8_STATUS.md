# Phase 8 Status — Realtime Plugin Graph / PDC / JUCE Studio Migration

Phase 8 is **IN PROGRESS**. This document records the first realtime-plugin-graph submilestone without claiming the full Phase 8 completion criteria.

## Delivered in this submilestone

- `IPluginProcessor` now has an explicit opt-in realtime contract for callback-safe block processing, latency reporting and reset.
- External processor construction, preparation and opaque-state restore happen while a new engine graph is published, not inside the audio callback.
- `RealtimePluginChain` owns prepared processor instances and preallocated scratch storage.
- `RealtimeDelayLine` provides preallocated fixed-sample delay storage used to align the dry path with reported wet-path latency.
- Built-in FLOW processors and prepared external effects can execute through the same realtime chain abstraction.
- The AudioEngine master path now executes a prepared realtime plugin chain after arrangement/preview/input-monitor summing.
- The JUCE backend reports `AudioProcessor::getLatencySamples()` and provides realtime VST3 effect processing through the backend-neutral host contract.
- Deterministic fake-latency tests cover exact delay, chain latency, latency-aligned wet/dry mixing and the AudioEngine master path.
- A real JUCE-built VST3 fixture is scanned, instantiated and processed inside the AudioEngine master realtime graph in CI.
- Project format remains v10; realtime processor state and latency metadata do not add portable project schema.

## Still required before Phase 8 is complete

- Per-track and per-bus realtime processor chains.
- Topology-aware PDC across direct routes, track-to-bus routes, sends and the final master sum.
- Explicit control-thread retirement/destruction of old external processor graphs rather than indefinite graph retention.
- Offline render/export parity with realtime external-plugin routing.
- Callback-safe true peak/RMS meters for track, bus and master.
- Remaining editing-surface migration from X11 to JUCE.
- Windows/macOS JUCE CI, AU runtime validation and platform packaging.
- External instrument-plugin MIDI routing; this submilestone validates external audio effects only.

## Realtime invariants

- FLOWDAW does not create plugin processors from the audio callback.
- FLOWDAW host scratch/delay buffers used by this submilestone are allocated during graph preparation.
- The callback does not take the engine publish mutex.
- Unsupported realtime processors are omitted from the prepared chain and surfaced as preparation issues rather than instantiated lazily from the callback.
- The X11 Studio remains available until the JUCE Studio reaches functional parity.
