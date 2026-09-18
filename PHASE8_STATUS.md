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
- Track, bus and master routes publish callback-safe sample-peak and RMS readings through lock-free 32-bit atomics; meter objects are allocated with the graph, not from the callback.
- JUCE reads meter snapshots on its UI timer and surfaces master metering while retaining route-ID readings for the upcoming Mixer UI.
- Track, bus and master meters now include a callback-safe 4x cubic inter-sample true-peak estimate with fixed three-sample history per channel; no allocation or lock is introduced in the callback. This is an engineering estimate for peak safety, not yet a standards-certified BS.1770/EBU implementation.

- Track-level external VST3/AU instrument slots now persist independently from mixer inserts; Pattern MIDI is scheduled as sample-offset note-on/note-off events during graph publication and rendered into a dedicated instrument buffer before track inserts.
- The JUCE backend exposes callback-safe MIDI input through the backend-neutral processor contract, and CI builds a real VST3 synth fixture to verify MIDI-to-audio routing end-to-end.
- Selecting an external instrument never silently falls back to a native FLOW instrument if the plugin fails to prepare.

- The JUCE Studio now has its first real editing surface: a track Arrangement with audio/pattern blocks, moving playhead, 1/16-snapped drag editing and one Undo entry per completed drag.
- JUCE transport now exposes play/pause, stop and undoable BPM edits.
- The selected-track mixer surface exposes volume, pan, mute, solo and a live stereo sample/true-peak meter fed from route-ID snapshots. Mixer slider gestures collapse to one Undo entry while intermediate values are republished from the control thread.
- JUCE Piano Roll now edits persistent Pattern MIDI directly: click-to-add, snapped drag, right-edge resize and Delete-to-remove, all through the shared Project/Undo/AudioEngine publication path with scale/grid rendering.
- JUCE Sampler/Chop now renders the imported waveform and persistent slice markers, previews slices/pads through the realtime preview engine, supports 16-pad banks, non-destructive boundary dragging, double-click split and Delete-to-merge with Undo.
- JUCE Step Sequencer now exposes 16-step paging over 16/32/64-step patterns, step activation, Velocity/Probability/Microtiming, lane Volume/Pan/Mute/Solo and pattern Swing/Humanize. Slider gestures collapse to one shared Undo entry and pattern length changes resize every lane persistently.
- JUCE Automation/Assist now writes Track/Bus/Master volume/pan points at the actual engine playhead, visualizes the selected automation lane, clears lanes non-destructively through Undo, and exposes Bus Volume/Pan/Mute/Solo plus Mix Bus creation.
- Production Assistant suggestions, workflow commands and Project Health validation are surfaced directly from the existing core APIs; actionable changes require an explicit Apply/Run action and enter the shared Undo stack.

## Still required before Phase 8 is complete

- JUCE Arrangement now also supports block selection, Delete, Duplicate and Pattern repeat count edits through the shared Undo stack.
- JUCE plugin routing now allows the scanned effect selection to be added as a Track insert or Master insert, and the Automation/Bus panel can create/update/remove Track→Bus sends with gain plus pre/post-fader mode through Undo.
- Remaining editing-surface migration from X11 to JUCE is limited to deeper rack/parameter-management polish rather than core production routing.
- Windows/macOS JUCE CI, AU runtime validation and platform packaging.

## Realtime invariants

- FLOWDAW does not create plugin processors from the audio callback.
- FLOWDAW host scratch/delay buffers used by this submilestone are allocated during graph preparation.
- The callback does not take the engine publish mutex.
- Graph destruction and external processor destruction never run from the realtime callback; JUCE periodically collects safe retired graphs from its control-thread timer.
- Unsupported realtime processors are omitted from the prepared chain and surfaced as preparation issues rather than instantiated lazily from the callback.
- External instrument instances, opaque-state restore and MIDI capacity preparation happen before graph publication; the callback only consumes precomputed note events and fixed-capacity scratch. Project format v11 keeps this source instrument separate from track effect inserts.
- The X11 Studio remains available until the JUCE Studio reaches functional parity.
