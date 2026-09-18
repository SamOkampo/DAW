# Phase 8 Status — Realtime Plugin Graph / PDC / JUCE Studio Migration

**Status: DONE — the JUCE Studio is now the production FLOWDAW desktop path, with the X11 shell retained only as an opt-in regression/bootstrap target.**

## Delivered

### Realtime plugin graph and routing
- External processors are instantiated, prepared and restored on the control thread before graph publication.
- Track, bus and master routes use prepared realtime plugin chains with preallocated scratch storage.
- Retired graphs/processors are reclaimed from the control thread only after realtime readers have released them.
- Real VST3 effects execute on track, bus and master routes; AU execution is validated on macOS.
- Persistent bypass, wet/dry and opaque plugin state are preserved by the shared `PluginInstance` model.
- External instrument slots consume scheduled MIDI before track inserts without falling back silently to FLOW instruments.
- Topology-aware PDC aligns direct track outputs, track→bus routes, sends, bus→master paths and plugin latency.
- Offline rendering, master WAV export and stem export use the same prepared routing/plugin behavior as realtime playback.

### Metering and realtime safety
- Track, bus and master meters publish sample peak, RMS and a fixed-cost 4x cubic inter-sample true-peak estimate.
- Device callbacks do not instantiate plugins, acquire the publish mutex or allocate host scratch/delay buffers.
- Unsupported processors fail during graph preparation and surface issues instead of being lazily created in the callback.

### JUCE Studio parity
- Transport: Play/Pause/Stop, BPM, Undo/Redo, New/Open/Save.
- Arrangement: audio/pattern blocks, playhead, snapped movement, selection, Delete, Duplicate and repeat edits.
- Step Sequencer: 16/32/64 steps, paging, Velocity, Probability, Microtiming, lane Volume/Pan/Mute/Solo, Swing/Humanize, native FLOW drum assignment and Straight/Boom Bap/Loose presets.
- Smart Sampling: WAV import, BPM/beat-grid analysis, Equal/Auto/Beat/Bar Chop, editable slice boundaries, multi-bank pads, Rename/Gain/Pan/Choke, keyboard pad performance and Stop Preview.
- REC CHOPS: persistent `ChopEvent` capture plus reversible 1/8–1/32 grid, Quantize 0–100%, Humanize 0–100% and Reset Feel.
- Match BPM: non-destructive WSOLA-derived asset workflow.
- Piano Roll: persistent MIDI notes, add/move/resize/delete, velocity/length, 1/8–1/32 grid, root/scale guidance, keyboard preview and octave navigation.
- Native instruments: FLOW Keys, FLOW 808, FLOW Bass and FLOW Lead with gain, pan, attack, release, tone, drive and tempo-aware delay controls.
- Recording: bounded realtime-safe audio capture, input monitoring, persistent takes and active-take selection.
- Mixer/Automation: track controls, buses, sends, pre/post mode, route meters and Track/Bus/Master volume/pan automation.
- Plugin workflow: VST3/AU scanning, external instruments/effects, Master/Track racks, FLOW Gain/Soft Clip/Width, bypass/remove, wet mix and native parameters.
- External rack editor restores the selected insert's persistent opaque state, republishes audible changes and collapses the editor session into one Undo entry.
- Assist, Project Health and explicit workflow commands are available from the JUCE Studio.
- A starter project gives a new session native drums plus a FLOW Keys melodic track so the user can create immediately.
- Master mix and per-track stems export from JUCE through the production PluginHost.

### Cross-platform validation
- Linux validates core, legacy X11 regression shell, JUCE desktop and real VST3 fixtures.
- Windows validates the JUCE desktop, real VST3 host path, install smoke test and portable ZIP packaging.
- macOS validates JUCE, real VST3 and AU fixtures, install smoke test and DMG packaging.
- The legacy X11 Studio remains source-available and CI-covered but is no longer the default product shell.

## Phase 8 exit decision

All Phase 8 roadmap criteria are implemented: prepared realtime plugin execution, graph lifetime safety, topology PDC, realtime/offline parity, route metering, functional JUCE editing/workflow parity and cross-platform production validation.

The remaining X11 target is intentionally **legacy opt-in**, not a second product UI. It is retained for regression/bootstrap value while the JUCE application is the production desktop path.

## Realtime invariants

- Plugin creation, state restore and preparation never occur in the audio callback.
- Host scratch, plugin and PDC delay storage are allocated during graph preparation.
- The audio callback does not take the engine publish mutex.
- Retired external processors are destroyed from the control thread.
- External instrument MIDI event capacity is prepared before publication.
- Project latency/PDC metadata remains runtime-only; Phase 8 does not add a new portable project format solely for runtime latency.
