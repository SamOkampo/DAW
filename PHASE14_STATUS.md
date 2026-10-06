# Phase 14 Status — Mastering / Loudness / Delivery

**Status: DONE**

## Invariants

- JUCE 9.0.2 production desktop.
- `.flow` remains v11.
- Mastering/export settings remain outside portable project state.
- Realtime safety contract is unchanged.
- Standards target is ITU-R BS.1770-5 + EBU R128/Tech 3341; no third-party certification claim.

## Phase structure

- **14.1 Standards contract + mastering analysis model — DONE.** PR #153 passed the full FLOWDAW CI matrix and merged as `a352df30`; K-weighted Integrated/Momentary/Short-term loudness, LRA, sample peak and offline 4x true-peak analysis are integrated.
- **14.2 Loudness normalisation / true-peak ceiling — DONE.** PR #153 merged as `a352df30`; static programme gain targets -23 LUFS by default, respects the configured true-peak ceiling and explicitly reports peak-constrained outcomes without hidden dynamics.
- **14.3 Export bit depth + dithering policy — DONE.** PR #153 merged as `a352df30`; Float32 remains undithered while PCM24/PCM16 support deterministic TPDF dither by default for float-to-integer delivery.
- **14.4 Mastering meter productization — DONE.** PR #154 passed the full FLOWDAW CI matrix and merged as `4cebace1`; production JUCE exposes offline Master Analysis while the callback meter remains a low-latency realtime estimate.
- **14.5 Delivery workflow / compliance report — DONE.** PR #154 merged as `4cebace1`; legacy Float32/stems remain available and the EBU-oriented PCM24+TPDF -23 LUFS/-1 dBTP delivery path reports achieved loudness, true peak, applied gain and peak-limited status.
- **14.6 Mastering regression audit — DONE / PASS.** PR #155 passed the full FLOWDAW CI matrix and merged as `8bf5be1b`; multi-rate loudness/true-peak, NaN/Inf safety, dither/export behavior and architecture/realtime/`.flow` compatibility are audited in `PHASE14_6_AUDIT.md`.
- **14.7 Final mastering closure — DONE / PASS.** PR #155 merged as `8bf5be1b`; the `.flow` v11 → render → analyze → PCM24 TPDF target export → decode → re-analyze golden path passed and `main` was verified after merge.

See `PHASE14_DESIGN.md`.

## Final closure

Phase 14 is complete and audited on `main`.

Ordered green merge chain:
- 14.1–14.3 → PR #153 → `a352df30`
- 14.4–14.5 → PR #154 → `4cebace1`
- 14.6–14.7 → PR #155 → `8bf5be1b`

Each implementation head passed core tests, legacy X11 smoke, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package and macOS JUCE/VST3/AU/install/DMG validation before merge.

Final invariants:
- production desktop remains JUCE 9.0.2;
- portable project format remains `.flow` v11;
- standards-grade mastering analysis/export runs offline/control-path, not in the realtime callback;
- legacy realtime meters remain callback-safe estimates;
- no third-party ITU/EBU certification is claimed;
- no Phase 15 is started by this closure.

The duplicate experimental PR line #156–#159 was closed without merge after the ordered block chain became authoritative.