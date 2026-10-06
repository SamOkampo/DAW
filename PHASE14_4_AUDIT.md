# Phase 14.4 Audit — Mastering Measurement / Export Regression

**State: IMPLEMENTED / PRE-CLOSURE**

Scope:
- K-weighted gated integrated loudness, momentary and short-term maxima, LRA;
- 4x band-limited true-peak estimation;
- Float32 / PCM24 / PCM16 delivery;
- TPDF dithering when reducing to integer PCM;
- EBU R128-style -23 LUFS / -1 dBTP target profile;
- existing realtime meter regression;
- sample-rate stability, NaN/Inf containment and deterministic seeded dither.

Realtime safety:
- MasteringAnalyzer is offline/control-thread only.
- PCM quantization/dither occurs during file export, never on the realtime audio callback.
- Existing RealtimeMeterState remains allocation-free in process().
- No filesystem, logging, UI, locks or dynamic allocation are added to the audio callback.

Standards boundary:
- Reference targets are ITU-R BS.1770-5 and EBU R128/Tech 3341 concepts.
- FLOWDAW does not claim external EBU/ITU conformance certification.
- The realtime TP display remains explicitly labelled a live estimate; the authoritative mastering report is full-program offline analysis.

Compatibility:
- .flow remains v11.
- Existing exportProjectWav remains Float32/backward-compatible.
- Existing VST3/AU and native plugin graph behavior is unchanged.
