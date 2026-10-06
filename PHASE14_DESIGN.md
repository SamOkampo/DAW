# Phase 14 Design — Loudness / True-Peak / Dither / Mastering Metering

**State: AUTHORIZED / IN PROGRESS**

Phase 14 closes the mastering-measurement/export gap left explicitly by Phase 13.

## Standards target

FLOWDAW Phase 14 targets:

- **ITU-R BS.1770-5 (11/2023)** for programme loudness and true-peak measurement.
- **EBU R 128 v5.0 (11/2023)** for the -23 LUFS programme target and -1 dBTP production ceiling for linear audio.
- **EBU Tech 3341 v4.0 (11/2023)** for EBU-mode loudness-meter presentation concepts.
- EBU production/distribution guidance requiring dither when reducing digital resolution.

This is an implementation/conformance target, **not a claim of third-party ITU/EBU certification**.

## Non-negotiable invariants

- Production desktop remains JUCE 9.0.2.
- Portable project format remains `.flow` v11.
- Master/export settings are delivery state, not song state; no project schema bump.
- Offline mastering analysis may allocate and read complete rendered audio.
- Realtime callback remains free of filesystem access, locks, logging, UI, scanning/instantiation and dynamic allocation.
- Existing fast Track/Bus/Master realtime meters remain callback-safe.
- External VST3/AU hosting, routing, PDC and Phase 13 native DSP must not regress.

## Sequence

### 14.1 — Standards contract + mastering analysis model

Add explicit programme metrics:
- Integrated loudness (LUFS) with 400 ms blocks, 75% overlap, -70 LUFS absolute gate and -10 LU relative gate.
- Maximum Momentary loudness (400 ms).
- Maximum Short-term loudness (3 s).
- Loudness Range descriptor using gated short-term loudness.
- Maximum sample peak dBFS.
- Maximum 4x oversampled true-peak dBTP.

### 14.2 — Loudness normalisation / true-peak ceiling

Provide static-gain programme normalisation:
- EBU R128 default target -23.0 LUFS.
- Maximum production true-peak -1.0 dBTP.
- Explicit report when true-peak headroom prevents reaching target loudness.
- Never silently add extra dynamics/limiting during normalisation.

### 14.3 — Export bit depth + dithering policy

Master delivery formats:
- 32-bit float WAV: no dither.
- 24-bit PCM WAV: TPDF dither by default when quantising from float.
- 16-bit PCM WAV: TPDF dither by default when quantising from float.
- deterministic TPDF implementation for reproducible tests.
- mastering export report records pre/post metrics, gain, target outcome, bit depth and dither mode.

### 14.4 — Mastering meter productization

Expose an offline Master Analysis action in the production JUCE Studio. Present:
- Integrated LUFS
- Momentary max
- Short-term max
- LRA
- true peak dBTP
- sample peak dBFS

The existing callback meter remains the low-latency playback meter; mastering analysis runs outside the audio callback.

### 14.5 — Delivery workflow / compliance report

Add a production export option for an EBU R128-oriented 24-bit TPDF master:
- target -23 LUFS;
- maximum -1 dBTP;
- visible report of achieved LUFS, TP, applied gain and peak-limited target status.

Keep legacy float32 Master Mix and Track Stems available.

### 14.6 — Mastering regression audit

Audit:
- 44.1 / 48 / 96 kHz analysis;
- loudness calibration/gating;
- inter-sample true-peak detection;
- silence/NaN/finite safety;
- normalisation target/ceiling interaction;
- PCM16/24 dither;
- export decode/roundtrip;
- no callback-safety regression;
- no project schema change.

### 14.7 — Final mastering closure

Golden path:
project → native/external processing → offline render → Master Analysis → optional EBU target normalisation → 24-bit TPDF export → decode → re-analyse.

Required closure matrix remains core tests, legacy X11, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package, macOS JUCE/VST3/AU/install/DMG.

## Claim boundary

Phase 14 may claim an internally validated BS.1770-5 / EBU R128-aligned mastering workflow after closure. It must not claim certification by ITU, EBU, a broadcaster, streaming platform or independent test laboratory.
