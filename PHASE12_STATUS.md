# Phase 12 Status — Native Sound Library & Preset System

**Status: IN PROGRESS**

Phase 12 is explicitly authorized after the completed Phase 11 closure. Its goal is to make a fresh FLOWDAW installation immediately musical with legally redistributable native sounds and presets.

## Non-negotiable invariants

- JUCE 9.0.2 production desktop.
- .flow v11 remains compatible unless an explicit tested migration is approved.
- No filesystem, manifest parsing, content scanning, WAV decoding, UI work, locks, waits or new dynamic allocation is introduced into the audio callback.
- Bundled library paths are installation-local; portable projects must not persist absolute install paths.
- User Browser roots/favorites/recents remain machine-local.
- Every shipped sound requires verified redistribution provenance.

## Phase structure

- **12.1 Core Library architecture — DONE.** PR #131 passed the full FLOWDAW CI matrix and merged as `adf62aeb`; versioned manifest, stable IDs, safe relative-path resolution and deterministic core tests are integrated.
- **12.2 FLOW Core drum/sample library — IMPLEMENTED / CI GATE.** Twelve deterministic first-party 48 kHz WAVs cover Kicks, Snares/Claps, Hats, Percussion, 808s and FX. Build-time generation, stable IDs/tags, provenance, decode/signal/determinism tests and Linux/Windows/macOS install-tree checks are included. Mark DONE only after this PR passes the full CI matrix and merges.
- **12.3 Native instrument preset catalog — PENDING.**
- **12.4 Browser integration — PENDING.**
- **12.5 Starter kits/templates — PENDING.**
- **12.6 Packaging/install/discovery — PENDING.**
- **12.7 Licensing/content-integrity audit — PENDING.**
- **12.8 Final integration/closure audit — PENDING.**

See `PHASE12_DESIGN.md` for acceptance criteria.

## Current checkpoint

12.1 is integrated. 12.2 implementation is complete and awaiting its own full FLOWDAW CI gate; 12.3 must not start before the 12.2 green merge.
