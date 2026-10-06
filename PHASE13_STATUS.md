# Phase 13 Status — Native Plugins Suite

**Status: DONE**

Phase 13 is authorized after the completed/audited Phase 12 closure.

## Invariants

- JUCE 9.0.2 production desktop.
- `.flow` remains v11.
- Native DSP is constructed/prepared off the realtime callback.
- No filesystem, locks, logging, UI, scanning/instantiation or dynamic allocation in realtime processing.
- Existing native inserts and external VST3/AU workflows must remain compatible.
- Mastering/loudness compliance remains reserved for Phase 14.

## Phase structure

- **13.1 Native DSP foundation + FLOW EQ — DONE.** PR #146 passed the full FLOWDAW CI matrix and merged as `f336269c`. Stateful prepared native processors, six-band FLOW EQ, generic native parameter metadata/rack editing and offline/realtime/`.flow` v11 regressions are integrated.
- **13.2 FLOW Compressor — DONE.** PR #148 passed the full FLOWDAW CI matrix and merged as `7f41b0d0`; stereo-linked feed-forward compression with threshold/ratio/attack/release/knee/makeup and regression coverage is integrated.
- **13.3 FLOW Limiter — DONE.** PR #149 passed the full FLOWDAW CI matrix and merged as `b4b77974`; sample-peak ceiling/input gain/lookahead/release limiting, preallocated lookahead state and PDC latency reporting are integrated. True-peak/LUFS compliance remains Phase 14.
- **13.4 FLOW Saturator — DONE.** PR #150 passed the full FLOWDAW CI matrix and merged as part of `02a2e4f1`; drive, tone and three saturation modes are integrated.
- **13.5 FLOW Reverb — DONE.** PR #150 merged as part of `02a2e4f1`; stereo algorithmic reverb with room/decay/damping/pre-delay/width and preallocated delay state is integrated.
- **13.6 FLOW Delay — DONE.** PR #150 merged as part of `02a2e4f1`; free-time/BPM delay, feedback/filtering and ping-pong behavior are integrated.
- **13.7 FLOW Chorus — DONE.** PR #150 merged as part of `02a2e4f1`; modulated fractional delay chorus with rate/depth/base delay/feedback/width is integrated.
- **13.8 FLOW Gate / Expander — DONE.** PR #150 merged as part of `02a2e4f1`; threshold/range/attack/hold/release gating is integrated.
- **13.9 FLOW Utility — DONE.** PR #150 merged as part of `02a2e4f1`; gain, polarity, mono, swap, balance and width utilities are integrated.
- **13.10 Native plugin presets — DONE.** PR #151 passed the full FLOWDAW CI matrix and merged as `759bf809`; versioned `FLOWDAW_EFFECT_PRESET 1` presets cover the Phase 13 native suite with save/load/instantiation and `.flow` v11 tests.
- **13.11 Native plugin editing/productization — DONE.** PR #151 merged as `759bf809`; the production JUCE rack exposes native effect selection, semantic parameter editing, compatible preset selection/application, Undo, graph republish and accessibility metadata.
- **13.12 DSP/realtime regression audit — DONE / PASS.** PR #152 passed the full FLOWDAW CI matrix and merged as `d2329753`; multi-sample-rate/block-size, NaN/Inf, reset, bypass, wet/dry, latency and parameter-bound coverage is recorded in `PHASE13_12_AUDIT.md`.
- **13.13 Final integration/closure audit — DONE / PASS.** PR #152 merged as `d2329753`; the native-only `.flow` v11 → render → WAV export golden path passed and `main` was verified after merge.

See `PHASE13_DESIGN.md`.


## Final closure

Phase 13 is complete and audited on `main`.

Ordered green merge chain:
- 13.1 → PR #146 → `f336269c`
- 13.2 → PR #148 → `7f41b0d0`
- 13.3 → PR #149 → `b4b77974`
- 13.4–13.9 → PR #150 → `02a2e4f1`
- 13.10–13.11 → PR #151 → `759bf809`
- 13.12–13.13 → PR #152 → `d2329753`

The final closure head passed core tests, legacy X11 smoke, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package, and macOS JUCE/VST3/AU/install/DMG validation before merge.

Final invariants:
- production desktop remains JUCE 9.0.2;
- `.flow` remains v11;
- native processor construction/allocation/preparation stays outside the audio callback;
- external VST3/AU workflows remain compatible;
- Phase 13 makes no LUFS, mastering-grade true-peak or dithering-policy claim.

Phase 14 is not started by this closure.
