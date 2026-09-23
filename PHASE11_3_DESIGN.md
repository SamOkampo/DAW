# Phase 11.3 Design — Arrangement + Browser Workspace

**State: DESIGNED — implementation not started.**

Phase 11.3 productizes the existing Arrangement and Sample Browser as one coherent creative workspace. It is a UI/UX subphase: existing editing semantics, async preview/import behavior, Undo/Redo, audio architecture and `.flow` v11 compatibility are invariants.

## Product intent

The Arrange workspace should read immediately as music-production software: tracks and timeline dominate, clips have clear musical state, and the Browser feels attached to the act of arranging rather than like a generic file-management card. FLOWDAW's rose/indigo/aqua visual language, notched silhouettes, restrained glow and depth should carry into this workspace without sacrificing dense DAW ergonomics.

## Existing dependencies to preserve

- `ArrangementComponent` already owns audio/pattern hit testing, bounded multi-selection, grouped Delete/Duplicate/repeat edits, horizontal scroll, Ctrl/Cmd+wheel zoom, 1/16 snapping, Alt free-drag and playhead presentation.
- `SampleBrowserComponent` already owns roots, search, Favorites/Recent, keyboard navigation, async WAV decode/preview, Auto Preview, import and internal `flowdaw-sample:` drag descriptions.
- Browser preview decoding/filesystem work remains outside the audio callback.
- Browser-to-workspace drop continues through the existing import route; Phase 11.3 must not create a second import or project-state path.
- Existing Project/Undo publication remains the only musical edit path.

## Bounded implementation sequence

### 11.3.1 — Arrangement visual hierarchy and track/clip state

First implementation block. Restyle only the Arrangement surface using the Phase 11 theme language:

- clearer ruler/timeline hierarchy and bar/beat contrast;
- professional track-header zone distinct from clip canvas;
- stronger but non-distracting selected/hovered clip states;
- visually distinct audio vs Pattern clips without relying only on labels;
- clearer playhead and drag/snap guide;
- concise contextual editing hint rather than tutorial-style permanent copy;
- empty-track/empty-arrangement visual state that remains useful rather than decorative.

No editing semantics change in this block.

### 11.3.2 — Browser visual hierarchy and selection state

Apply the shared theme to Browser shell, roots/search/filter controls, rows, favorite state, preview status and empty states. Preserve async preview, search/filter semantics and keyboard shortcuts exactly.

### 11.3.3 — Arrangement/Browser integration and drag/drop affordance

Make the Browser feel spatially connected to Arrangement and expose an intentional drop target/feedback state while reusing the existing `flowdaw-sample:` drag/import route. No filesystem/decode work may move into drag handlers or audio callback.

### 11.3.4 — 11.3 regression/closure audit

Verify visual hierarchy, editing semantics, Browser behavior, Undo/Redo, project compatibility and realtime safety; run the required multiplatform CI before marking 11.3 DONE.

## 11.3.1 acceptance criteria

1. Arrangement ruler, track headers and clip canvas have visibly different hierarchy at normal desktop density.
2. Audio clips and Pattern clips have FLOWDAW-specific visual identities and remain readable when selected.
3. Selection remains obvious for single and Ctrl/Cmd multi-selection.
4. Playhead, snapped drag guide and Alt free-drag state remain distinguishable.
5. Existing Delete, Duplicate, repeat adjustment, drag, scroll and zoom behavior is unchanged.
6. No new Project fields, serializer changes or project-format bump.
7. No DSP/audio callback/routing/plugin-host change.
8. Theme treatment must not resemble a generic SaaS card/dashboard: use the established music-software identity intentionally, with dense timeline geometry taking priority over ornamental panels.
9. Existing core/JUCE/legacy regression jobs required by the repository remain green before merge.

## 11.3.2 acceptance criteria

1. Browser search, roots, Recent/Favorites/Auto Preview and primary actions have clear hierarchy and visible focus/selected/disabled states.
2. Rows communicate selected and favorite state without losing filename/path readability.
3. Loading/preview/error/empty states are visually distinct.
4. Space/Enter/Escape, arrows, Home/End and `/` search behavior remain intact.
5. WAV decode and filesystem traversal remain off the audio callback; async generation cancellation remains intact.
6. No AppSettings schema change unless separately justified, migrated and tested; the intended block requires none.

## 11.3.3 acceptance criteria

1. Dragging a Browser WAV into Arrangement provides clear target feedback before drop.
2. Successful drop uses the existing import path and creates no duplicate state/import implementation.
3. Invalid/non-FLOWDAW drag descriptions are rejected cleanly.
4. Drop feedback is UI-only and introduces no filesystem/decode/audio work into realtime processing.
5. Browser remains usable independently through Import/Enter/double-click.

## Tests / verification

For each implementation PR:

- compile production JUCE Studio;
- existing core tests;
- legacy X11 smoke where required by CI;
- Linux JUCE runtime/plugin fixture/install/package path;
- Windows JUCE job;
- macOS JUCE VST3/AU job;
- code review of diff boundary for AudioEngine/serialization/routing/plugin-host changes;
- `.flow` remains v11.

11.3 closure additionally exercises the existing Phase 10 golden-path Arrangement and Browser interactions: multi-select + grouped Undo/Redo, scroll/zoom/snap/free placement, Browser search/Recent/Favorites/navigation/preview/Auto Preview/import, and save/reopen persisted musical state.

## Explicit exclusions

- No Mixer or plugin-rack redesign (11.4).
- No Piano Roll, Sequencer, Sampler or Automation redesign (11.5).
- No general resizable-panel/adaptive-layout system or broad accessibility closure (11.6).
- No device/plugin-scan/settings/diagnostics relocation (11.7).
- No new DSP, mastering, routing, buses, sends, metering algorithms or export behavior.
- No new audio/sample formats.
- No `.flow` schema change.

## Realtime-safety gate

Phase 11.3 must not add filesystem access, locks, logging, UI calls, plugin scanning/instantiation, waits, or new dynamic allocation/preparation work to the audio callback. Arrangement painting/input stays on the UI thread; Browser traversal/decode/preview preparation stays on existing UI/worker/control paths.

## Next implementation step

Implement **only 11.3.1 Arrangement visual hierarchy and track/clip state** on a fresh branch based on the integrated design. Do not start Browser restyling or drag/drop integration in that implementation PR.