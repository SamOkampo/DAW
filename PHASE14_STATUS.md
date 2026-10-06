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
- **14.6 Mastering regression audit — PENDING.**
- **14.7 Final mastering closure — PENDING.**

See `PHASE14_DESIGN.md`.

## Current checkpoint

14.1–14.3 are the active clean CI gate (#153). 14.4–14.5 are implemented on the ordered descendant branch and must merge only after the mastering core is green/integrated.
