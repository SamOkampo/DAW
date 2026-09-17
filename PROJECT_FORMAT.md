# FLOWDAW project format

Current project format: **v8**.

`.flow` is a versioned text format so development remains inspectable and migratable. The loader accepts v1-v8 and normalizes loaded projects to v8 in memory.

## Sample / Smart Sampling model

Each `SampleAsset` can persist:

- original filesystem path or native sound key;
- detected BPM and confidence;
- `sourceSampleId` and `timeRatio` for derived Match-BPM assets;
- zero or more non-destructive `SampleSlice` ranges.

Each v7+ `SampleSlice` persists:

- stable slice ID;
- editable name;
- `startFrame` / `endFrame`;
- per-pad Gain;
- per-pad Pan;
- Choke Group (`0` = no choke).

Older v1-v6 projects load with safe defaults for these pad controls: gain `1.0`, pan `0.0`, choke group `0`.

An original asset has `sourceSampleId = 0`. A Match-BPM asset stores a non-zero source ID and a duration ratio. On reopen FLOWDAW loads the original source, then regenerates the derived buffer with the stored time-stretch ratio. The original file is never overwritten.

## Chop performance model

`ChopEvent` persists its sample/slice identity together with original captured `recordedTick` / `recordedVelocity` and current edited `tick` / `velocity`.

Patterns persist:

- Chop quantize grid;
- 0–100% Chop quantize strength;
- 0–100% deterministic Chop humanize.

Editing is non-cumulative: rendered timing/velocity is rebuilt from the captured performance, so changing or resetting quantize/humanize does not repeatedly move the source performance.

## MIDI / instrument model — v8

A `Pattern` can now contain zero or more persistent `MidiNote` events. Each note stores:

- stable event ID;
- `startTick`;
- `lengthTicks`;
- MIDI pitch `0..127`;
- velocity.

The same pattern also stores its native `InstrumentState`:

- enabled state and instrument type;
- gain and pan;
- attack and release;
- tone and drive;
- tempo-synced delay mix and delay tick length.

Piano Roll state persisted with the pattern includes scale root, scale type, MIDI grid and default note length. Supported scale identifiers currently include `major`, `minor`, `major_pentatonic` and `minor_pentatonic`.

v1-v7 projects load with an empty MIDI event list and the safe default instrument state. They remain editable and can be saved as v8 without rewriting source audio.

## Sequencer / Arrangement model

Patterns persist step count, subdivisions, Swing, Humanize, drum lanes, Chop events and MIDI notes. Drum lanes persist Volume, Pan, Mute and Solo. Pattern placements persist musical start tick and repeat count.

MIDI instrument patterns use the same `PatternPlacement` Arrangement scheduling model as drums and recorded chops. This keeps musical timing in ticks until the audio graph is published, where MIDI notes are rendered at the current project BPM.

## Compatibility rule

Persisted schema changes must increment `formatVersion` and retain explicit backward loading where practical. v8 currently preserves backward loading through v1.
