# Phase 14 Status — Mastering / Loudness / Delivery

**Status: IN PROGRESS**

## Invariants

- JUCE 9.0.2 production desktop.
- `.flow` remains v11.
- Mastering/export settings remain outside portable project state.
- Realtime safety contract is unchanged.
- Standards target is ITU-R BS.1770-5 + EBU R128/Tech 3341; no third-party certification claim.

## Phase structure

- **14.1 Standards contract + mastering analysis model — IMPLEMENTED / CI GATE.** K-weighted programme loudness, absolute/relative gating, Momentary/Short-term maxima, LRA, sample peak and offline 4x true-peak metrics.
- **14.2 Loudness normalisation / true-peak ceiling — IMPLEMENTED / CI GATE.** Static-gain target normalisation with explicit true-peak-constrained outcome.
- **14.3 Export bit depth + dithering policy — IMPLEMENTED / CI GATE.** Float32, PCM24 and PCM16 WAV delivery with TPDF default for integer reductions plus mastering export reports.
- **14.4 Mastering meter productization — IMPLEMENTED / STACKED CI CANDIDATE.** Studio exposes offline Master Analysis with persistent Integrated/Momentary/Short-term/LRA/true-peak summary while the callback meter remains the low-latency playback meter.
- **14.5 Delivery workflow / compliance report — IMPLEMENTED / STACKED CI CANDIDATE.** Export menu retains Float32 Master Mix/Stems and adds an EBU R128-oriented PCM24+TPDF -23 LUFS/-1 dBTP delivery path with achieved LUFS/TP and peak-constrained status.
- **14.6 Mastering regression audit — IMPLEMENTED / PRE-CLOSURE.** Multi-rate loudness/true-peak, NaN/Inf safety, deterministic dither, Float32 no-dither and architecture/realtime/project-compatibility evidence is recorded in `PHASE14_6_AUDIT.md`.
- **14.7 Final mastering closure — IMPLEMENTED / PRE-CLOSURE.** End-to-end `.flow` v11 → render → analysis → PCM24 TPDF target export → decode → re-analysis golden path is implemented in `tests/test_phase14_golden_path.cpp` and documented in `PHASE14_7_AUDIT.md`.

See `PHASE14_DESIGN.md`.

## Current checkpoint

#153 gates 14.1–14.3, #154 gates 14.4–14.5, and this branch is the final 14.6–14.7 closure candidate. Phase 14 remains IN PROGRESS until all current-head CI gates pass and merge in order.