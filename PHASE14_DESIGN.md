# Phase 14 Design — Loudness, True Peak, Dither & Mastering Metering

**State: AUTHORIZED / IMPLEMENTED / CI GATE**

Phase 14 closes the mastering-measurement/export gap left intentionally open by Phases 11–13.

## Standards baseline

- ITU-R BS.1770-5 (11/2023) — programme loudness and true-peak measurement.
- EBU R 128 v5.0 (11/2023) — programme loudness target and descriptors.
- EBU Tech 3341 v4.0 (11/2023) — EBU Mode meter behavior and minimum-requirement test signals.
- EBU Tech 3342 — Loudness Range descriptor.

FLOWDAW implements a standards-aligned internal mastering path. It does not claim third-party certification.

## Non-negotiable invariants

- Production desktop remains JUCE 9.0.2.
- Portable project format remains `.flow` v11; mastering delivery options are export-time choices, not song-schema fields.
- Heavy K-weighting, gating, LRA, oversampled true-peak analysis and normalization stay outside the realtime callback.
- The realtime meter remains a lightweight callback-safe estimate and is explicitly labeled `TP EST`.
- No filesystem, locks, logging, UI calls, scanning, processor construction or dynamic allocation is added to the audio callback.
- Existing Float32 master/stem export remains backward compatible.
- External VST3/AU and Phase 13 native DSP remain unchanged.

## 14.1 — BS.1770 / EBU Mode offline mastering analysis

Provide:
- K-weighted programme loudness;
- 400 ms Momentary loudness;
- 3 s Short-Term loudness;
- Integrated loudness with -70 LUFS absolute gate and -10 LU relative gate;
- Loudness Range using short-term distribution and relative gating;
- sample peak dBFS;
- oversampled offline true-peak dBTP;
- compliance result against an explicit mastering target.

Acceptance includes the EBU Tech 3341 minimum Test 1: stereo 1 kHz sine at -23 dBFS peak/channel reports M/S/I = -23.0 ±0.1 LUFS.

## 14.2 — Export bit depth and dithering policy

Policy:
- Float32 WAV: no dither; no integer quantization.
- PCM24 WAV: TPDF enabled by default when reducing from the internal floating path.
- PCM16 WAV: TPDF enabled by default.
- Dither is applied exactly once at final integer quantization.
- No noise shaping is silently applied in Phase 14.
- Deterministic seed support exists for regression tests/reproducible exports.

Mastering export is two-pass:
1. render and analyze;
2. apply gain-only loudness normalization, constrained by measured true-peak ceiling;
3. re-analyze;
4. quantize/write using selected delivery format.

It never hides a limiter in the export path. If the target loudness cannot be reached without violating true peak, the export remains lower and the report says so.

## 14.3 — Productization / mastering meter surface

The production `Export…` menu adds:
- Analyze Master — LUFS-I, max LUFS-M, max LUFS-S, LRA, dBTP;
- Mastering WAV PCM24 + TPDF — R128 target;
- Mastering WAV PCM16 + TPDF — R128 target.

The R128 mastering target is -23 LUFS with ±0.5 LU tolerance and a FLOWDAW delivery ceiling of -1 dBTP.

The realtime header meter is relabeled `TP EST` so users cannot confuse its lightweight callback-safe estimate with the offline standards-aligned true-peak measurement.

## 14.4 — Regression and closure

Required:
- EBU reference-vector loudness test;
- gain/LU linearity;
- silence/gating behavior;
- true peak >= sample peak;
- target normalization and true-peak constraint;
- deterministic TPDF;
- undithered silence remains zero;
- PCM16/PCM24 readback;
- end-to-end project mastering export;
- Linux/Windows/macOS production CI;
- architecture/realtime and `.flow` v11 audit.

## Claim boundary

Phase 14 may describe FLOWDAW as providing standards-aligned loudness/true-peak analysis and mastering delivery tools.

It must not claim that FLOWDAW itself has been independently certified by ITU, EBU or another laboratory. The official EBU Loudness Test Set can be used later as an external certification/acceptance suite without changing the project format or realtime architecture.
