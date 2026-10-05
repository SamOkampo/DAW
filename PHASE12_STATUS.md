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

- **12.1 Core Library architecture — IN PROGRESS.** Versioned manifest, stable IDs, safe relative-path resolver and core tests.
- **12.2 FLOW Core drum/sample library — PENDING.**
- **12.3 Native instrument preset catalog — PENDING.**
- **12.4 Browser integration — PENDING.**
- **12.5 Starter kits/templates — PENDING.**
- **12.6 Packaging/install/discovery — PENDING.**
- **12.7 Licensing/content-integrity audit — PENDING.**
- **12.8 Final integration/closure audit — PENDING.**

See `PHASE12_DESIGN.md` for acceptance criteria.
