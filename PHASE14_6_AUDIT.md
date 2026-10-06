# Phase 14.6 Audit — Mastering / Loudness / Realtime Safety

**State: IMPLEMENTED / PRE-CLOSURE**

## Standards scope

Phase 14 targets ITU-R BS.1770-5 (11/2023), EBU R128 v5.0 and EBU Tech 3341 concepts for programme loudness and true-peak presentation.

FLOWDAW's internal conformance target is not a third-party ITU/EBU certification.

## Loudness implementation

Automated coverage validates:

- K-weighted programme measurement;
- 400 ms Momentary blocks;
- 3 s Short-term windows;
- -70 LUFS absolute gating;
- -10 LU relative integrated-loudness gating;
- Loudness Range calculation from gated short-term distributions;
- 44.1, 48 and 96 kHz analysis;
- digital silence behavior;
- finite handling when source buffers contain NaN/Inf.

## Peak implementation

- sample peak is reported as dBFS;
- mastering true-peak uses offline 4x band-limited polyphase interpolation;
- inter-sample test material verifies true peak can exceed sample peak;
- EBU-oriented export ceiling defaults to -1 dBTP.

The legacy realtime Track/Bus/Master meter remains a callback-safe fast 4x cubic estimate and is explicitly presented as a realtime estimate, not the mastering compliance measurement.

## Normalisation policy

Optional programme normalization applies one static gain to the entire rendered programme.

- default target: -23.0 LUFS;
- default max true peak: -1.0 dBTP;
- if the gain required to hit -23 LUFS would exceed the true-peak ceiling, gain is capped and the report explicitly marks `limitedByTruePeak`;
- no hidden compressor/limiter is introduced by loudness normalization.

## Dither / bit-depth policy

- Float32 WAV: no dither or integer quantisation.
- PCM24: TPDF dither by default when reducing from float.
- PCM16: TPDF dither by default when reducing from float.
- deterministic PRNG seed is intentional for reproducible automated regression.
- audit verifies undithered digital silence stays zero and TPDF produces bounded LSB-scale decorrelation.

## Realtime safety

PASS by architecture:

- full-program loudness analysis, true-peak analysis, normalisation and dithering are offline/export/control-path work;
- none of Phase 14's standards processing is called by the realtime audio callback;
- no project graph/router/plugin/PDC changes are required;
- `.flow` remains v11 because mastering delivery choices are not persisted song state.

## Remaining closure gate

Phase 14 is not DONE until the ordered implementation PRs pass the full core/X11/Linux/Windows/macOS matrix, merge, and the final golden path is verified on `main`.
