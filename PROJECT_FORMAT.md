# FLOWDAW project format

Current project format: **v9**.

`.flow` is a versioned text format so development remains inspectable and migratable. The loader accepts v1-v9 and normalizes loaded projects to v9 in memory.

## Sample / Smart Sampling model

Each `SampleAsset` can persist its source path or native key, detected BPM/confidence, derived-asset source/ratio metadata and non-destructive `SampleSlice` ranges. v7+ slices persist stable ID, editable name, frame range, Gain, Pan and Choke Group. Match-BPM assets regenerate from their original source and ratio instead of overwriting source audio.

## Chop performance model

`ChopEvent` stores sample/slice identity, original captured `recordedTick` / `recordedVelocity` and current edited timing/velocity. Patterns persist Chop quantize grid, strength and deterministic humanize, so feel edits can always be rebuilt from the captured performance.

## MIDI / instrument model — v8+

A `Pattern` can contain persistent `MidiNote` events with ID, start tick, length, MIDI pitch and velocity. The pattern also persists `InstrumentState`: instrument type, gain/pan, attack/release, tone/drive and tempo-synced delay. Piano Roll scale root/type, grid and default note length are also stored.

## Recording / take model — v9

Each `Track` can persist:

- record-arm state;
- input-monitor state;
- zero or more `RecordingTake` records;
- one `activeTakeId` used as the current non-destructive comp choice.

A take stores a stable ID, editable take name, `sampleId`, musical `startTick` and `lengthTicks`. The audio itself remains a normal WAV-backed `SampleAsset`; selecting another take changes the active comp without rewriting or deleting any recorded source file.

## Advanced mixer / routing — v9

Tracks persist an `outputBusId` (`0` means Master) plus zero or more `MixerSend` entries. Each send stores its stable ID, destination bus, gain, enabled state and pre/post-fader mode.

The project can persist `Bus` objects with their own mixer Volume, Pan, Mute, Solo and insert-effect list. Existing gain effects remain compatible with track, bus and master channels.

## Automation — v9

The project persists zero or more `AutomationLane` objects. Each lane stores target name, target ID, optional sub-target ID and ordered musical control points (`tick`, `value`). Duplicate ticks are normalized deterministically and playback uses linear interpolation between points.

Current engine targets include:

- `track.volume`
- `track.pan`
- `bus.volume`
- `bus.pan`
- `send.gain` (`targetId = track.id`, `subTargetId = send.id`)
- `master.volume`

Automation remains in musical ticks until audio graph evaluation so it follows project BPM consistently.

## Sequencer / Arrangement model

Patterns persist step count, subdivisions, Swing, Humanize, drum lanes, Chop events and MIDI notes. Pattern placements persist musical start and repeat count. Audio clips, active takes, drum patterns, chops and MIDI instruments all enter the same published render graph before mixer routing and automation are applied.

## Compatibility rule

Persisted schema changes increment `formatVersion` and keep explicit backward loading where practical. v9 preserves backward loading through v1; older projects receive safe defaults for recording, buses, sends and automation and can be resaved as v9 without modifying their source audio.
