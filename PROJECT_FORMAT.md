# FLOWDAW project format

Current project format: **v7**.

`.flow` is a versioned text format so development remains inspectable and migratable. The loader accepts v1-v7 and normalizes loaded projects to v7 in memory.

## Sample / Smart Sampling model

Each `SampleAsset` can persist:

- original filesystem path or native sound key;
- detected BPM and confidence;
- `sourceSampleId` and `timeRatio` for derived Match-BPM assets;
- zero or more non-destructive `SampleSlice` ranges.

Each v7 `SampleSlice` persists:

- stable slice ID;
- editable name;
- `startFrame` / `endFrame`;
- per-pad Gain;
- per-pad Pan;
- Choke Group (`0` = no choke).

Older v1-v6 projects load with safe defaults for these new pad controls: gain `1.0`, pan `0.0`, choke group `0`.

An original asset has `sourceSampleId = 0`. A Match-BPM asset stores a non-zero source ID and a duration ratio. On reopen FLOWDAW loads the original source, then regenerates the derived buffer with the stored time-stretch ratio. The original file is never overwritten.

## Chop performance model

`ChopEvent` persists its sample/slice identity together with original captured `recordedTick` / `recordedVelocity` and current edited `tick` / `velocity`.

Patterns persist:

- Chop quantize grid;
- 0–100% Chop quantize strength;
- 0–100% deterministic Chop humanize.

Editing is non-cumulative: rendered timing/velocity is rebuilt from the captured performance, so changing or resetting quantize/humanize does not repeatedly move the source performance.

## Sequencer model

Patterns persist step count, subdivisions, Swing, Humanize, lanes and per-step Active/Velocity/Probability/Microtiming. Lanes persist Volume, Pan, Mute and Solo. Pattern placements persist musical start tick and repeat count.

## Compatibility rule

Persisted schema changes must increment `formatVersion` and retain explicit backward loading where practical. v7 currently preserves backward loading through v1.
