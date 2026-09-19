# Phase 9 Status — Workflow / Browser / Polish / Autosave

**Status: IN PROGRESS**

Phase 8 established JUCE as the production desktop path. Phase 9 now focuses on daily workflow safety and speed without changing the realtime DSP architecture.

## Milestone 9.1 — JUCE autosave and crash recovery

Implemented on this branch:

- The JUCE Studio owns the existing `SessionRecovery` service in the machine-local FLOWDAW settings directory.
- A dirty recovery snapshot is restored at startup when `restoreLastSession` is enabled.
- The active project gets an immediate recovery snapshot and periodic snapshots at `AppSettings::autosaveSeconds`.
- New Project, Open Project and Save Project refresh recovery metadata for the newly confirmed session.
- A crash-recovered session remains protected if the user closes the app without explicitly choosing Save, New or Open; the last recovery snapshot is therefore not silently destroyed.
- Once the user explicitly confirms the session with Save/New/Open, a later clean shutdown removes the dirty recovery marker and autosave files.
- Autosave runs on the JUCE message/control thread; the audio callback performs no recovery file I/O or project mutation.
- Phase 6 recovery tests cover non-consuming recovery loads, repeated snapshots, generation increments, latest project metadata and latest project state.

## Next

1. Sample/file Browser with search.
2. Favorites and recent samples.
3. Drag/drop into the production workspace.
4. Templates and first-run workflow.
5. Keyboard shortcuts and command palette.
6. UX hierarchy/polish while preserving the realtime architecture.

Project format remains **v11**.
