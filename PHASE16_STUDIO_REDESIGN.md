# Phase 16 — FLOWDAW Studio Redesign

Status: PLANNED (design only). No claim of functional implementation or physical QA.

## Objective

Rebuild the JUCE desktop workflow into a coherent music-production studio: fewer persistent controls, context-aware toolbars, integrated workspace views, a compact browser/mixer layout, and a distinctive graphite-and-muted-orange palette. FL Studio is a workflow reference, not a source to copy assets, code, trade dress, icons or exact UI.

This phase is separate from Phase 15 release readiness. PR #161 and issue #162 remain subject to JUCE licence, signing, notarization, complete packaged-app QA and exact-artifact provenance. Do not merge #161 or promote 1.0 without these gates.

## Non-negotiable invariants

- Keep JUCE 9.0.2, musical project .flow v11, autosave/recovery, Undo/Redo, routing/PDC, plugin-state persistence and audio/export semantics.
- No allocations, I/O, logging, locks, waits, plugin-editor operations or GUI calls in the real-time audio callback.
- All new UI arrangement, theme and panel state belongs to machine-local presentation settings, not musical projects.
- Implement in small independently tested PRs and require green Linux, Windows, macOS, core and legacy CI before merge. Physical GUI QA remains mandatory.
- Reconcile pending #163 adaptive-window and #164 midnight/copper styling; the final requested colors are GRAPHITE/AMBER, superseding blue/purple/mint. Review shared app/juce_main.cpp and app/JuceSampleBrowser.hpp rather than overwriting either change.
- Native and third-party plugin editors must obey JUCE/OS plugin ownership. Embed only if validated; otherwise a controlled native floating host window is an acceptable documented fallback.

## Design tokens — Graphite / Amber

| Role | Color |
| --- | --- |
| Main canvas | #101113 |
| Panel | #181A1D |
| Raised panel | #22252A |
| Hover | #2D3137 |
| Subtle border | #363A40 |
| Primary text | #ECECE8 |
| Secondary text | #B6B5AE |
| Muted text | #92918B |
| Amber primary | #C2783D |
| Amber hover | #E09755 |
| Amber deep | #7D4E2C |
| Play/success | #76AD76 |
| Record/clip | #BE6660 |
| Warning | #D4A15A |
| Keyboard focus | #E9B179 |

Use subtle near-flat controls with precise outlines, no loud gradients everywhere. Accent represents selection/active tools, not decoration. Preserve accessible contrast and visible focus.

## Technical delivery sequence

### 16.1 — UI inventory and baseline [PLANNED]
Document all existing toolbar/edit/record/sampler/rack/settings controls and their event handlers in app/juce_main.cpp. Record before screenshots on a real Windows install at 100/125/150% scaling; list genuinely corrupted labels and shortcuts. Map each command to an accessible destination before hiding anything.
Acceptance: complete control-to-command routing inventory; no functionality silently removed.

### 16.2 — Premium graphite / amber theme [PLANNED]
Refactor app/JuceTheme.hpp tokens and module-specific hardcoded colors. Cover Browser, Arrangement, Piano Roll, Sequencer, Mixer, native racks, popups and meters. Add color-contrast checks and focused/dimmed states.
Acceptance: consistent appearance on all creative surfaces and physical screenshots.

### 16.3 — Compact transport and action grouping [PLANNED]
Top bar: transport, record, BPM, project, Undo/Redo, small audio status, save. Move nonessential controls to File/Edit/View/Tools/Help and the existing Ctrl/Cmd+K palette. Preserve accessible labels/tooltips and shortcuts.
Acceptance: every existing command remains reachable, with discoverable menu or contextual surface.

### 16.4 — Workspace architecture [PLANNED]
Left collapsible sample/plugin browser; center tabbed Arrangement/Piano Roll/Sequencer/Sampler/Automation; bottom height-bounded dockable mixer; optional right selection Inspector; diagnostics/export/settings via dialogs or utility drawers. Reuse PR #163 viewport patterns for smaller desktops.
Acceptance: content reachable without overlap at 720x520, 1366x768, 1920x1080, 4K; verify scroll, maximize/restore and keyboard traversal.

### 16.5 — Integrated plugin and editor host [PLANNED]
Add an in-app editor/document manager for native tools with close/reopen/tab/focus management and stable document identities. For VST3/AU, embed only where tested and supported by the editor/OS contract; use a JUCE-owned floating wrapper fallback. Editor creation and teardown must be on the message/control thread.
Acceptance: open/close/reopen real VST3 fixture, maintain plugin state, focus and editor ownership; separately test AU on macOS.

### 16.6 — Contextual controls [PLANNED]
Replace permanent stacks of unrelated sampler/chop/plugin/routing buttons with active-editor toolbars, compact channel strips, FX slots, a selection inspector and overflow menus. Keep monitoring/recording actions discoverable when relevant.
Acceptance: real editing workflows with no lost operations or Undo/Redo regressions.

### 16.7 — Text, keyboard and accessibility [PLANNED]
Audit UTF-8, glyph fallbacks, font sizing, DPI and symbols visible in the user's Windows screenshot. Replace fragile decorative glyphs with drawn icons or plain terms. Keyboard focus must be visible; labels must be readable at 100/125/150% scaling.
Acceptance: no reproduced mojibake, tested icons/tooltips, keyboard and screen-reader metadata.

### 16.8 — Technical and physical regression [PLANNED]
Tests must actually run in Release (not silently disabled by NDEBUG). Verify shell geometry, menu action routing, document/editor lifecycle, background focus, save/reopen, crash recovery, real plugin hosting, transport, export and .flow v11 compatibility. Preserve full Linux/Windows/macOS JUCE matrix and audio callback invariants. Gather Windows, macOS and Linux installed-app physical evidence.
Acceptance: green CI and independently recorded physical QA. CI does not substitute human GUI validation.

### 16.9 — RC integration / final audit [PLANNED]
Integrate reviewed small PRs in dependency order; explicitly reconcile #163 and #164 files with Phase 15 versioned code so visible version never regresses to 0.8.0. Produce newly built verifiable release-candidate artifacts, not final 1.0. Packaging must exclude JUCE SDK sources, include third-party notices and document VC++ x64 runtime prerequisite.
Acceptance: verified commit/hash provenance and no unresolved blockers. Issue #162 alone governs final 1.0 promotion.

## Proposed first technical slice

Implement 16.1 UI inventory and 16.2 token consolidation as separate PRs; no large UI rewrite until their tests pass. Phase 16 is NOT completed by acceptance of this design.
