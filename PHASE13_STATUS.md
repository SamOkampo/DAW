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
- **13.2 FLOW Compressor — IMPLEMENTED / CI GATE.** Clean main-based candidate with stereo-linked feed-forward compression, generic rack parameter exposure and offline/realtime/block-size/`.flow` v11 coverage.
- **13.3 FLOW Limiter — PENDING.**
- **13.4 FLOW Saturator — PENDING.**
- **13.5 FLOW Reverb — PENDING.**
- **13.6 FLOW Delay — PENDING.**
- **13.7 FLOW Chorus — PENDING.**
- **13.8 FLOW Gate / Expander — PENDING.**
- **13.9 FLOW Utility — PENDING.**
- **13.10 Native plugin presets — PENDING.**
- **13.11 Native plugin editing/productization — PENDING.**
- **13.12 DSP/realtime regression audit — PENDING.**
- **13.13 Final integration/closure audit — PENDING.**

See `PHASE13_DESIGN.md`.
