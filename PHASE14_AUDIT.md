# Phase 14 Final Audit — Mastering Measurement & Delivery

**State: PRE-CLOSURE — PASS ONLY AFTER GREEN MERGE**

## Architecture / realtime

PASS by design inspection:
- `Mastering.cpp` is an offline analysis module and is not called by the audio callback.
- K-weighting, gating, loudness-range sorting, oversampled true-peak convolution and two-pass normalization allocate only in offline/control paths.
- Realtime `RealtimeMeterState` is not made heavier; its display is explicitly labeled `TP EST`.
- Existing immutable graph / PDC / native and external plugin paths are unchanged.

## Loudness / measurement

Implemented:
- BS.1770-equivalent De Man K-weight coefficient generation at arbitrary supported sample rates;
- EBU-style 400 ms blocks at 75% overlap for Integrated/Momentary;
- -70 LUFS absolute gate and -10 LU relative gate;
- 3 s Short-Term windows;
- LRA percentile descriptor with relative gating;
- sample peak and oversampled offline dBTP.

Regression includes EBU Tech 3341 minimum Test 1: stereo 1 kHz at -23 dBFS peak/channel must report M/S/I -23.0 ±0.1 LUFS.

## Dither / export

PASS by implementation:
- Float32 path remains unchanged and undithered;
- integer PCM16/PCM24 use final-stage TPDF when enabled;
- regression verifies deterministic seeded TPDF and zero-valued undithered silence;
- no noise shaping or hidden limiter is introduced;
- mastering normalization is gain-only and constrained by measured true peak.

## Compatibility

- Project format remains `.flow` v11.
- Mastering target, bit depth and dither are export-time choices and are not serialized into songs.
- Existing `exportProjectWav` and stem behavior remain available.
- External plugin and Phase 13 native-plugin state contracts are unchanged.

## Product path

`Export…` exposes analysis plus PCM24/PCM16 mastering exports. Result text surfaces LUFS-I, max M/S, LRA, dBTP, applied gain and whether the requested target was achieved.

## Certification boundary

This audit is a FLOWDAW engineering acceptance audit, not an external EBU/ITU certification. Passing the bundled reference/regression tests supports standards alignment; independent certification may additionally execute the official EBU Loudness Test Set.

## Closure gate

Phase 14 can be marked DONE only after the final head passes core tests, legacy X11 smoke, Linux JUCE, Windows JUCE and macOS JUCE/AU/install/package validation, merges to `main`, and `main` is reverified.
