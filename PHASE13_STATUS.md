# Phase 13 Status — Native Plugins Suite

**Status: IN PROGRESS**

Phase 13 is authorized after the completed/audited Phase 12 closure.

## Invariants

- JUCE 9.0.2 production desktop.
- `.flow` remains v11.
- Native DSP is constructed/prepared off the realtime callback.
- No filesystem, locks, logging, UI, scanning/instantiation or dynamic allocation in realtime processing.
- Existing native inserts and external VST3/AU workflows must remain compatible.
- Mastering/loudness compliance remains reserved for Phase 14.

## Phase structure

- **13.1 Native DSP foundation + FLOW EQ — IMPLEMENTED / CI GATE.** Stateful prepared native processors replace the old realtime per-sample shortcut; FLOW EQ provides six parametric bands + output gain, generic native parameter metadata, scalable rack native-insert/parameter selectors and offline/realtime/persistence tests.
- **13.2 FLOW Compressor — IMPLEMENTED / STACKED CI CANDIDATE.** Stereo-linked feed-forward compression with threshold/ratio/attack/release/knee/makeup, callback-safe prepared state, generic rack parameter exposure, offline/realtime parity, block-size independence and `.flow` v11 persistence tests. Must merge only after 13.1 is green and integrated.
- **13.3 FLOW Limiter — IMPLEMENTED / STACKED CANDIDATE.** Sample-peak limiter foundation with ceiling/input gain/lookahead/release, preallocated delay state, latency reporting for PDC, generic native parameter exposure, offline/realtime parity, block-size independence and `.flow` v11 persistence tests. True-peak mastering compliance remains Phase 14.
- **13.4 FLOW Saturator — IMPLEMENTED / STACKED CANDIDATE.** Drive, tone and three saturation modes using prepared state with finite-output coverage.
- **13.5 FLOW Reverb — IMPLEMENTED / STACKED CANDIDATE.** Stereo algorithmic feedback network with room/decay/damping/pre-delay/width and preallocated delay state.
- **13.6 FLOW Delay — IMPLEMENTED / STACKED CANDIDATE.** Free-time or BPM/beat-derived delay, feedback, filtering and ping-pong behavior with preallocated storage.
- **13.7 FLOW Chorus — IMPLEMENTED / STACKED CANDIDATE.** Modulated fractional delay with rate/depth/base delay/feedback/width and preallocated storage.
- **13.8 FLOW Gate / Expander — IMPLEMENTED / STACKED CANDIDATE.** Stereo-linked threshold/range/attack/hold/release gate with deterministic envelope state.
- **13.9 FLOW Utility — IMPLEMENTED / STACKED CANDIDATE.** Gain, polarity, mono, swap, balance and width in one callback-safe insert.
- **13.10 Native plugin presets — IMPLEMENTED / STACKED CANDIDATE.** Versioned native effect preset format plus first-party preset coverage for EQ, Compressor, Limiter, Saturator, Reverb, Delay, Chorus, Gate and Utility with save/load and `.flow` v11 roundtrip tests.
- **13.11 Native plugin editing/productization — IMPLEMENTED / STACKED CANDIDATE.** Generic native insert selector, generic parameter selector and compatible preset selector/application workflow live in the production JUCE rack with undo/re-publish behavior and accessibility metadata.
- **13.12 DSP/realtime regression audit — IMPLEMENTED / PRE-CLOSURE.** Automated multi-sample-rate/block-size finite-output, reset, bypass, wet/dry, latency and parameter-bound coverage plus structural realtime audit in `PHASE13_12_AUDIT.md`.
- **13.13 Final integration/closure audit — IMPLEMENTED / PRE-CLOSURE.** Native-only production golden path covers preset-backed Track/Master racks, `.flow` v11 save/reopen, production offline render and master WAV export/readback. Closure still requires ordered green merges and final `main` verification.

See `PHASE13_DESIGN.md`.

## Current checkpoint

Implementation for 13.1–13.13 is complete on an ordered branch lineage. Phase 13 remains IN PROGRESS until the lineage is validated and merged in order with the required multiplatform CI, then final main verification updates this file to DONE.