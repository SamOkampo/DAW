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
- `RealtimePdcPlan` now computes topology-aware latency alignment for direct track outputs, track→bus paths and sends before those routes are moved into dedicated callback buffers.
- `RealtimeStereoRouteBuffer` provides preallocated stereo storage for the per-track/per-bus callback graph.
- `AudioEngine` now renders project sources into dedicated track buffers, executes prepared track inserts, routes pre/post-fader sends and track outputs into bus buffers, executes prepared bus inserts, and then sums latency-aligned routes into the master.
- Topology-aware PDC is applied to direct track outputs, track→bus routes, sends and bus→master paths; a deterministic impulse test verifies differently latent routes land on the same sample.
- JUCE CI now validates real VST3 processing not only on master but across a real track→bus insert chain.
- Graph ownership now separates the currently published graph from retired graphs; realtime/offline readers are counted atomically and retired plugin graphs are reclaimed only from the control thread when no reader can still reference them.
- A concurrent test holds a processor inside the realtime callback while a replacement graph is published, proving the old graph is retained until the callback exits; repeated idle publications are also verified not to accumulate retired graphs.
- Offline rendering now drives the same prepared track/bus/master realtime graph in bounded blocks, preserving external effects, PDC, automation and master processing instead of using the legacy flattened renderer.
- Project WAV export and per-track stem export accept the production `PluginHost`, so JUCE-backed VST3 effects are instantiated for bounce; CI compares deterministic realtime/offline samples and also bounces the real JUCE VST3 fixture.

## Still required before Phase 8 is complete

- Offline render/export parity with realtime external-plugin routing.
- Callback-safe true peak/RMS meters for track, bus and master.
- Remaining editing-surface migration from X11 to JUCE.
- Windows/macOS JUCE CI, AU runtime validation and platform packaging.
- External instrument-plugin MIDI routing; this submilestone validates external audio effects only.

## Realtime invariants

- FLOWDAW does not create plugin processors from the audio callback.
- FLOWDAW host scratch/delay buffers used by this submilestone are allocated during graph preparation.
- The callback does not take the engine publish mutex.
- Graph destruction and external processor destruction never run from the realtime callback; JUCE periodically collects safe retired graphs from its control-thread timer.
- Unsupported realtime processors are omitted from the prepared chain and surfaced as preparation issues rather than instantiated lazily from the callback.
- The X11 Studio remains available until the JUCE Studio reaches functional parity.
