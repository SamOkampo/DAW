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
- **13.2 FLOW Compressor — DONE.** PR #148 passed the full FLOWDAW CI matrix and merged to `main`; stereo-linked feed-forward dynamics, generic rack parameters and offline/realtime/block-size/`.flow` v11 coverage are integrated.
- **13.3 FLOW Limiter — DONE.** PR #149 passed the full FLOWDAW CI matrix and merged as `b4b77974`; sample-peak ceiling/input gain/lookahead/release limiting, preallocated lookahead delay, explicit latency for PDC and `.flow` v11 coverage are integrated. True-peak/LUFS compliance remains Phase 14.
- **13.4 FLOW Saturator — IMPLEMENTED / CI GATE.** Drive, tone and three saturation modes with prepared state and finite-output coverage.
- **13.5 FLOW Reverb — IMPLEMENTED / CI GATE.** Stereo algorithmic feedback reverb with room/decay/damping/pre-delay/width and preallocated delay state.
- **13.6 FLOW Delay — IMPLEMENTED / CI GATE.** Free-time or BPM/beat-derived delay with feedback/filtering/ping-pong and preallocated storage.
- **13.7 FLOW Chorus — IMPLEMENTED / CI GATE.** Modulated fractional delay with rate/depth/base delay/feedback/width and preallocated storage.
- **13.8 FLOW Gate / Expander — IMPLEMENTED / CI GATE.** Stereo-linked threshold/range/attack/hold/release gate with deterministic envelope state.
- **13.9 FLOW Utility — IMPLEMENTED / CI GATE.** Gain, polarity, mono, swap, balance and width in one callback-safe insert.
- **13.10 Native plugin presets — IMPLEMENTED / STACKED CI CANDIDATE.** Versioned `FLOWDAW_EFFECT_PRESET 1` format plus first-party presets for EQ, Compressor, Limiter, Saturator, Reverb, Delay, Chorus, Gate and Utility with save/load, instantiation and `.flow` v11 roundtrip tests.
- **13.11 Native plugin editing/productization — IMPLEMENTED / STACKED CI CANDIDATE.** Production JUCE rack exposes native effect selection, semantic parameter selection, compatible preset selection/application, Undo and graph republish with accessibility metadata.
- **13.12 DSP/realtime regression audit — PENDING.**
- **13.13 Final integration/closure audit — PENDING.**

See `PHASE13_DESIGN.md`.
