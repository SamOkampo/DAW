# FLOWDAW project format

Current project format: **v4**.

`.flow` is a versioned text format so early development remains inspectable and migratable. Older v1-v3 files are accepted by the loader and normalized to v4 in memory.

## v4 sample model

Each `SampleAsset` can persist:

- original filesystem path or native sound key;
- detected BPM and confidence;
- `sourceSampleId` and `timeRatio` for derived Match-BPM assets;
- zero or more non-destructive `SampleSlice` ranges (`startFrame`, `endFrame`).

An original asset has `sourceSampleId = 0`. A Match-BPM asset stores a non-zero source ID and a duration ratio. On reopen FLOWDAW loads the original source, then regenerates the derived buffer with the stored time-stretch ratio. The original file is never overwritten.

## Sequencer model

Patterns persist step count, subdivisions, Swing, Humanize, lanes and per-step Active/Velocity/Probability/Microtiming. Lanes persist Volume, Pan, Mute and Solo. Pattern placements persist musical start tick and repeat count.

## Forward rule

Future changes must increment `formatVersion` when the persisted schema changes and keep explicit migration/backward-loading behavior where practical.
