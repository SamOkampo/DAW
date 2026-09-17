# REC CHOPS — Project Format v5

REC CHOPS stores sample-pad performances as musical events rather than destructive audio clips.

Each `ChopEvent` contains:

- `tick`: position relative to the owning Pattern.
- `sampleId`: source or derived `SampleAsset`.
- `sliceId`: one persisted non-destructive `SampleSlice`.
- `velocity`: event gain multiplier.
- `pan`: event pan offset.

The owning Pattern is placed in Arrangement through the existing `PatternPlacement` model, so the whole take can be moved, repeated and later quantized without modifying the original WAV.

Project format v5 remains backward-readable for v1-v4 files. v5 adds zero or more `CHOP` rows after each Pattern's drum-lane data.
