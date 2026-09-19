# FLOWDAW project format

Current project format: **v11**.

`.flow` is a versioned text format so development remains inspectable and migratable. The loader accepts v1-v11 and normalizes loaded projects to v11 in memory.

## Sample / Smart Sampling model

Each `SampleAsset` can persist its source path or native key, detected BPM/confidence, derived-asset source/ratio metadata and non-destructive `SampleSlice` ranges. v7+ slices persist stable ID, editable name, frame range, Gain, Pan and Choke Group. Match-BPM assets regenerate from their original source and ratio instead of overwriting source audio.

## Chop performance model

`ChopEvent` stores sample/slice identity, original captured `recordedTick` / `recordedVelocity` and current edited timing/velocity. Patterns persist Chop quantize grid, strength and deterministic humanize, so feel edits can always be rebuilt from the captured performance.

## MIDI / instrument model — v8+

A `Pattern` can contain persistent `MidiNote` events with ID, start tick, length, MIDI pitch and velocity. The pattern also persists `InstrumentState`: instrument type, gain/pan, attack/release, tone/drive and tempo-synced delay. Piano Roll scale root/type, grid and default note length are also stored.

## Recording / take model — v9+

Each `Track` can persist:

- record-arm state;
- input-monitor state;
- zero or more `RecordingTake` records;
- one `activeTakeId` used as the current non-destructive comp choice.

A take stores a stable ID, editable take name, `sampleId`, musical `startTick` and `lengthTicks`. The audio itself remains a normal WAV-backed `SampleAsset`; selecting another take changes the active comp without rewriting or deleting any recorded source file.

## Advanced mixer / routing — v9+

Tracks persist an `outputBusId` (`0` means Master) plus zero or more `MixerSend` entries. Each send stores its stable ID, destination bus, gain, enabled state and pre/post-fader mode.

The project can persist `Bus` objects with their own mixer Volume, Pan, Mute, Solo, legacy insert-effect list and v10 plugin rack.

## Automation — v9+

The project persists zero or more `AutomationLane` objects. Each lane stores target name, target ID, optional sub-target ID and ordered musical control points (`tick`, `value`). Duplicate ticks are normalized deterministically and playback uses linear interpolation between points.

Current engine targets include:

- `track.volume`
- `track.pan`
- `bus.volume`
- `bus.pan`
- `send.gain` (`targetId = track.id`, `subTargetId = send.id`)
- `master.volume`

Automation remains in musical ticks until audio graph evaluation so it follows project BPM consistently.

## Plugin racks — v10

`MasterMixer` and each `MixerChannel` can persist an ordered `PluginInstance` rack. Each instance stores:

- stable plugin ID;
- format (`builtin`, `vst3`, `au`, or a future backend identifier);
- stable identifier and display name;
- source path when applicable;
- enabled and bypass state;
- wet/dry mix;
- ordered parameter ID/value pairs;
- opaque plugin state string for backend-specific state serialization.

Built-in FLOW processors currently include Gain, Soft Clip and Width. External plugin slots use the same persistent model, but actual third-party binary execution is delegated to a registered `IExternalPluginBackend`. The project format therefore does not depend on JUCE, Steinberg VST3 SDK classes or Audio Unit APIs.

External slots discovered by the current X11 bootstrap are inserted bypassed by default until a production backend is registered. This makes reopen/migration deterministic and prevents the project format from claiming processing that did not occur.

## Save-safety behavior — v10

Project saves are written through a temporary sibling file and then moved into place. If a previous project file exists, its prior contents are retained as `<project>.bak` before replacement. The backup is not part of the `.flow` schema itself; it is a recovery aid for the save operation.

## Sequencer / Arrangement model

Patterns persist step count, subdivisions, Swing, Humanize, drum lanes, Chop events and MIDI notes. Pattern placements persist musical start and repeat count. Audio clips, active takes, drum patterns, chops and MIDI instruments all enter the same published render graph before mixer routing, automation and supported plugin processing are applied.

## Compatibility rule

Persisted schema changes increment `formatVersion` and keep explicit backward loading where practical. The v11 loader preserves backward loading through v1. Older projects receive safe defaults for recording, buses, sends, automation, plugin racks and external instrument slots and can be resaved as v11 without modifying their source audio.


## External instrument slot — v11+

A Track may persist one external instrument source separately from its mixer insert rack. The slot stores the same backend-neutral `PluginInstance` identity, parameters and opaque state used by effect racks. Piano Roll `MidiNote` events remain in Patterns; PatternPlacements schedule note-on/note-off events into the Track instrument at sample offsets during graph publication. Projects v1-v10 load with the external instrument slot disabled.
