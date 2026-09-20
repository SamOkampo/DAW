# Phase 9 Status — Workflow / Browser / Polish / Autosave

**Status: DONE**

Phase 9 closes the production-workflow gap left after the JUCE Studio migration while preserving the realtime architecture and project compatibility.

## 9.1 — Autosave / recovery

- JUCE Studio owns the existing `SessionRecovery` service.
- Dirty recovery is restored when configured, snapshots refresh periodically and after confirmed project transitions.
- Recovery I/O remains on the message/control thread.
- Existing recovery lifecycle regression coverage remains active.

## 9.2 — Sample Browser

- Persistent machine-local sample folders.
- Permission-safe, bounded recursive WAV discovery.
- Search by filename/path.
- Imports route through the existing project edit/import path.

## 9.3 — Favorites / Recent / drag & drop

- AppSettings v3 persists Favorites and Recent samples with v1/v2 migration.
- Favorites can be toggled directly in the Browser and are visibly marked.
- Successful Browser imports are kept in a bounded, de-duplicated Recent list.
- Browser WAVs expose an internal `flowdaw-sample:` drag payload.
- The production workspace accepts only that internal WAV payload and routes it through `importWavFile`.
- No Browser filesystem scanning, drop validation or WAV decoding runs in the audio callback.

## 9.4 — Templates / onboarding

- New-project menu offers Blank (120 BPM), Boom Bap (90 BPM), Trap (140 BPM) and Lo-Fi (82 BPM) starting points.
- Fresh installs receive a first-run workflow guide for templates, Browser drag/drop and Commands.

## 9.5 — Commands / shortcuts / polish

- Ctrl/Cmd+K opens a command palette.
- Shortcuts cover New, Open, Save, Import WAV, Play/Pause, Undo/Redo and the five editor views.
- FLOWDAW Studio uses a clearer dark workspace hierarchy and active editor-tab state.

## Compatibility and safety

- Production desktop path: **JUCE 9.0.2**.
- Project format remains **v11**.
- Phase 9 introduces no project-schema migration and no new realtime filesystem work.
- Phase 9 is complete only after the final Linux/Windows/macOS + core/legacy CI matrix is green.
