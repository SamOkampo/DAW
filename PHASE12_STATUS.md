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
- **12.3 Native instrument preset catalog — IMPLEMENTED / CI GATE.** Clean main-based PR #135 contains ten versioned `.flowpreset` files for Keys, Bass, 808, Lead and Pad roles with parser/range, audible-render and install checks. Merge only after its full CI rerun is green.
- **12.4 Browser integration — IMPLEMENTED / STACKED CI CANDIDATE.** FLOW Core is a distinct first-party Sample Browser source; sample queries match stable ID/path/category/tags, rows expose FLOW categories, and existing preview/import/drag/favorites/recent behavior is reused without audio-thread filesystem work. Build-root wiring exercises the UI now; robust installed/package location remains explicitly reserved for 12.6.
- **12.5 Starter kits/templates — IMPLEMENTED / STACKED CI CANDIDATE.** Blank, Boom Bap, Trap and Lo-Fi builders now use portable `content:<id>` references plus FLOW Core presets, preserve `.flow` v11, rehydrate first-party audio on startup/recovery/open, retain legacy native-drum loading, and include save→reopen tests proving package/build roots are not serialized.
- **12.6 Packaging/install/discovery — IMPLEMENTED / STACKED CI CANDIDATE.** Runtime discovery resolves Linux/Windows `share/FLOWDAW/...`, macOS app-bundle Resources and a development fallback; TGZ/ZIP/DMG CI inspects packaged content directly. Project format and realtime boundaries are unchanged.
- **12.7 Licensing/content-integrity audit — PENDING.**
- **12.8 Final integration/closure audit — PENDING.**

See `PHASE12_DESIGN.md` for acceptance criteria.

## Current checkpoint

12.1 and 12.2 are integrated. Clean PR #135 remains the active 12.3 gate. 12.4, 12.5 and 12.6 are prepared as sequential stacked validation PRs; mandatory merge order remains 12.3 → 12.4 → 12.5 → 12.6.
