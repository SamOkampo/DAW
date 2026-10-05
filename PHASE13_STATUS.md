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

- **13.1 Native DSP foundation + FLOW EQ — DONE.** PR #146 passed the full FLOWDAW CI matrix and merged to `main` as `f336269c`. Stateful prepared native processors replace the old realtime per-sample shortcut; FLOW EQ provides six parametric bands + output gain, generic native parameter metadata, scalable rack native-insert/parameter selectors and offline/realtime/persistence tests.
- **13.2 FLOW Compressor — IMPLEMENTED / CI GATE.** Clean PR #148 is the parent gate for this branch.
- **13.3 FLOW Limiter — IMPLEMENTED / CI GATE.** Clean PR #149 is the parent gate for this branch; true-peak/LUFS compliance remains Phase 14.
- **13.4 FLOW Saturator — IMPLEMENTED / STACKED CANDIDATE.** Drive, tone and three saturation modes with prepared state and finite-output coverage.
- **13.5 FLOW Reverb — IMPLEMENTED / STACKED CANDIDATE.** Stereo algorithmic feedback reverb with room/decay/damping/pre-delay/width and preallocated delay state.
- **13.6 FLOW Delay — IMPLEMENTED / STACKED CANDIDATE.** Free-time or BPM/beat-derived delay with feedback/filtering/ping-pong and preallocated storage.
- **13.7 FLOW Chorus — IMPLEMENTED / STACKED CANDIDATE.** Modulated fractional delay with rate/depth/base delay/feedback/width and preallocated storage.
- **13.8 FLOW Gate / Expander — IMPLEMENTED / STACKED CANDIDATE.** Stereo-linked threshold/range/attack/hold/release gate with deterministic envelope state.
- **13.9 FLOW Utility — IMPLEMENTED / STACKED CANDIDATE.** Gain, polarity, mono, swap, balance and width in one callback-safe insert.
- **13.10 Native plugin presets — IMPLEMENTED / STACKED CANDIDATE.** Versioned native effect preset format plus first-party preset coverage for EQ, Compressor, Limiter, Saturator, Reverb, Delay, Chorus, Gate and Utility with save/load and `.flow` v11 roundtrip tests.
- **13.11 Native plugin editing/productization — IMPLEMENTED / STACKED CANDIDATE.** Production JUCE rack exposes native effect selection, semantic parameter selection, compatible preset selection/application, Undo and graph republish with accessibility metadata.
- **13.12 DSP/realtime regression audit — PENDING.**
- **13.13 Final integration/closure audit — PENDING.**

See `PHASE13_DESIGN.md`.
