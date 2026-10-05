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
- **12.2 FLOW Core drum/sample library — DONE.** PR #132 passed the full FLOWDAW CI matrix and merged as `71e55bb8`; twelve deterministic first-party 48 kHz WAVs, provenance, decode/signal/determinism tests and multiplatform install-tree checks are integrated.
- **12.3 Native instrument preset catalog — IMPLEMENTED / CI GATE.** Ten versioned `.flowpreset` files cover Keys, Bass, 808, Lead and Pad roles using the existing `flow_keys`, `flow_bass`, `flow_808` and `flow_lead` engines. Strict parser/range validation, audible-render regression and install-tree checks are included. Mark DONE only after this clean main-based PR passes the full FLOWDAW CI matrix and merges.
- **12.4 Browser integration — PENDING.**
- **12.5 Starter kits/templates — PENDING.**
- **12.6 Packaging/install/discovery — PENDING.**
- **12.7 Licensing/content-integrity audit — PENDING.**
- **12.8 Final integration/closure audit — PENDING.**

See `PHASE12_DESIGN.md` for acceptance criteria.

## Current checkpoint

12.1 and 12.2 are integrated. 12.3 is now the active clean main-based CI gate; 12.4 remains prepared separately and must not merge before the 12.3 green merge.
