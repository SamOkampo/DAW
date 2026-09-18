# Phase 8 Status — Realtime Plugin Graph / PDC / JUCE Studio Migration

**Status: DONE**

The JUCE Studio is the production FLOWDAW desktop path. The X11 shell is retained only as an explicit legacy regression/bootstrap target.

## Delivered

### Realtime plugin graph and routing

- External processors are instantiated, prepared and state-restored on the control thread before graph publication.
- Track, bus and master routes use prepared realtime plugin chains with preallocated host scratch/delay storage.
- Retired graphs/processors are reclaimed from the control thread only after realtime readers have released them.
- Real VST3 effects execute on Track/Bus/Master routes; AU execution is validated on macOS.
- Persistent bypass, wet/dry and opaque plugin state are preserved by the shared `PluginInstance` model.
- External instrument slots consume scheduled MIDI before Track inserts without silently falling back to FLOW instruments.
- Topology-aware PDC aligns direct Track outputs, Track→Bus routes, sends, Bus→Master paths and reported plugin latency.
- Offline rendering, master WAV export and stem export use the same prepared routing/plugin behavior as realtime playback.

### Metering and realtime safety

- Track, bus and master routes publish sample peak, RMS and a fixed-cost 4x cubic inter-sample true-peak estimate.
- Device callbacks do not instantiate plugins, acquire the publish mutex or allocate host scratch/PDC buffers.
- Unsupported processors fail during graph preparation and surface issues instead of being lazily created in the callback.
- Plugin/editor state capture and graph replacement occur from the control thread.

### JUCE Studio parity

- Transport: New/Open/Save, Play/Pause/Stop, BPM and Undo/Redo.
- Starter session: native drum beat plus FLOW Keys melodic Pattern.
- Arrangement: audio/pattern blocks, moving playhead, snap, selection, Delete, Duplicate and Pattern repeat edits.
- Step Sequencer: 16/32/64 steps, paging, Velocity, Probability, Microtiming, lane Volume/Pan/Mute/Solo, native FLOW drum assignment, Swing/Humanize and Straight/Boom Bap/Loose presets.
- Smart Sampling: WAV import, BPM/beat-grid analysis, Equal/Auto/Beat/Bar Chop, editable slices, multi-bank pads, Rename/Gain/Pan/Choke, keyboard performance and Stop Preview.
- REC CHOPS: persistent `ChopEvent` capture, reversible 1/8–1/32 grid, Quantize 0–100%, Humanize 0–100% and Reset Feel.
- Match BPM: non-destructive WSOLA-derived asset workflow.
- Piano Roll: persistent MIDI notes, add/move/resize/delete, velocity/length, musical grids, root/scale guidance, keyboard preview and octave navigation.
- Native instruments: FLOW Keys, FLOW 808, FLOW Bass and FLOW Lead with gain, pan, envelope, tone, drive and tempo-aware delay controls.
- Recording: bounded realtime-safe capture, input monitoring, persistent takes and active-take selection.
- Mixer/Automation: Track controls, buses, sends, pre/post-fader mode, route meters and Track/Bus/Master volume/pan automation.
- Plugin workflow: VST3/AU scanning, external instruments/effects, Track/Master racks, FLOW Gain/Soft Clip/Width, bypass/remove, wet mix and native parameters.
- External rack editor restores the selected persistent insert state, republishes audible changes and collapses the editor session into one Undo entry.
- Production Assistant, Project Health and explicit workflow commands are available from the JUCE Studio.
- Master mix and per-track stem export execute through the production PluginHost.

### Cross-platform production validation

- Linux validates core, legacy X11 regression shell, JUCE production desktop, real VST3 fixtures, installation and TGZ packaging.
- Windows validates JUCE, real VST3 host paths, installation and portable ZIP packaging.
- macOS validates JUCE, real VST3/AU effect+synth fixtures, installation and DMG packaging.
- Core-only CI explicitly disables JUCE, preserving toolkit independence.
- The X11 shell defaults OFF and is CI-covered only as legacy regression/bootstrap.

## Phase 8 exit decision

All Phase 8 roadmap criteria are satisfied:

1. Plugin creation/preparation/state restore stay off the audio callback.
2. Realtime Track/Bus/Master chains use preallocated graph resources.
3. Retired processor graphs are reclaimed from the control thread.
4. Real external effects run through the project mixer graph with persistent bypass/wet/state behavior.
5. Current-topology plugin delay compensation is active.
6. Deterministic fake-latency and reclamation tests validate alignment/lifetime behavior.
7. Real JUCE plugin fixtures execute through the AudioEngine graph.
8. Realtime and offline/export routing are aligned.
9. Callback-safe Track/Bus/Master metering is available.
10. Core production editing/workflows are available in JUCE.
11. X11 is retired from the default product path and remains only as a legacy target.
12. Linux/Windows/macOS CI and platform packaging validate the production runtime.

**Production desktop path: JUCE. Project format: v11. FLOWDAW version: 0.8.0.**

## Realtime invariants

- Plugin creation, state restore and preparation never occur in the audio callback.
- Host scratch, plugin and PDC delay storage are allocated during graph preparation.
- The audio callback does not take the engine publish mutex.
- Retired external processors are destroyed from the control thread.
- External instrument MIDI capacity is prepared before graph publication.
- Runtime latency/PDC metadata does not require a portable project-format field.
