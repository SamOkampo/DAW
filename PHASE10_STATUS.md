# Phase 10 Status — Production Editing / Mixer / Browser / Workflow

**Status: IN PROGRESS**

Phase 10 turns the broad production feature set completed through Phase 9 into a faster, clearer day-to-day music-production workflow. Work stays incremental: one bounded subpoint per branch/PR, green CI before merge, and no convenience feature may weaken the realtime contract.

## 10.1 — Arrangement editing

Planned sequence:
1. Establish explicit Arrangement selection state and predictable click-to-select behavior.
2. Add bounded multi-selection/edit operations without changing the project schema unless persistence is actually required.
3. Improve timeline navigation, zoom/scroll and musical snapping using existing 960 PPQ timing.
4. Refine clip/pattern manipulation feedback and Undo/Redo coverage.

## 10.2 — Mixer production workflow

- Clarify Track / Bus / Master channel-strip hierarchy and active target state.
- Make routing, sends and plugin-rack operations easier to inspect and edit.
- Preserve the existing immutable realtime routing/plugin graph and PDC behavior.

## 10.3 — Browser production workflow

- Add safe sample preview and faster navigation on top of the Phase 9 Browser.
- Keep filesystem traversal, decoding and preview preparation off the audio callback.
- Preserve the existing project import route and bounded machine-local history.

## 10.4 — Piano Roll / Sequencer workflow

- Improve selection and common musical editing operations.
- Keep edits command/Undo based and preserve deterministic timing/groove behavior.

## 10.5 — Plugin workflow

- Improve plugin discovery presentation, search/selection and rack ergonomics.
- Discovery, instantiation, editor/state work and failure handling remain outside the realtime callback.
- Preserve backend-neutral project state and quarantine behavior.

## 10.6 — Performance / robustness

- Profile hot paths before optimization.
- Add focused stress/regression coverage for any Phase 10 behavior that can affect playback or graph publication.
- Do not trade callback safety for UI responsiveness.

## 10.7 — Release-quality closure

- Cross-workflow polish and production golden-path validation.
- Full Linux / Windows / macOS production CI plus core and legacy regressions.
- Final audit of architecture, realtime safety, project compatibility and documentation before Phase 10 is marked DONE.

## Invariants

- Production desktop path remains **JUCE 9.0.2**.
- Project format begins Phase 10 at **v11** and remains v11 unless a specific approved feature genuinely requires persisted schema state; any migration must retain backward loading.
- Audio callback: no filesystem access, locks, logging, UI calls, plugin scanning/instantiation or new dynamic-allocation work.
- Project edits remain control-thread operations that publish realtime-safe state.
- PRs stay small and scoped; CI must be green before merge.
- Phase 11 does not begin until the Phase 10 final audit is complete.

## Current checkpoint

Completed in the parent branch / `main` sequence:
- **10.1** Arrangement editing (selection, multi-select Delete, navigation/zoom/snapping, manipulation feedback and Undo/Redo-safe resynchronization).
- **10.2.1** explicit Mixer active-target hierarchy for Track / Bus / Master with target-aware controls and meters.

This branch implements **10.2.2 — routing and sends inspection/edit UX**:
- When the active Mixer target is the selected Track, the channel strip now exposes its output route directly: Master or any existing Bus.
- The same Mixer surface exposes one selected send Bus with gain, Pre/Post state, Set/Update and Remove actions.
- Existing `Track::outputBusId` and `Track::sends` project state are reused; no schema or serialization change is introduced.
- Routing/send edits commit through the existing UndoStack and republish the prepared immutable routing graph on the control thread.
- Routing/send controls disable automatically for Bus/Master targets or when no valid Bus exists.
- The existing Automation/Assist surface remains compatible; this change promotes common routing work into the primary Mixer instead of removing advanced controls.

After this PR is green and merged, the next bounded Mixer subpoint is **10.2.3 — plugin-rack ergonomics and target synchronization**. Do not start Browser 10.3 until Mixer 10.2 is closed.
