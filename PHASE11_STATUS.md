# Phase 11 Status — UI/UX & Productization

**Status: IN PROGRESS**

Phase 11 starts only after the completed Phase 10 architecture/realtime/project-compatibility audit. Its job is to make the production JUCE application feel like a coherent desktop DAW without rewriting the audio engine or weakening project compatibility.

## Product objective

FLOWDAW should present a clear production path:

**create/import → arrange → edit → mix → master/export**

The interface should feel intentional, readable and fast rather than exposing implementation details as the primary workflow.

## Non-negotiable invariants

- Production desktop path remains **JUCE 9.0.2**.
- `.flow` remains **v11** unless a later explicitly approved subpoint genuinely requires persisted project state; any schema change must preserve backward loading.
- Phase 11 must not change DSP behavior merely to simplify UI work.
- Audio callback rules remain unchanged: no filesystem access, locks, logging, UI work, plugin scanning/instantiation, waits or new dynamic-allocation work.
- UI and filesystem work stay on message/control/background paths as appropriate.
- Existing Undo/Redo semantics, routing/PDC, plugin state, autosave/recovery and offline/realtime parity must remain intact.
- Small scoped PRs; green CI before merge.

## Phase structure

### 11.1 — FLOWDAW design system and shell architecture [DONE]

Centralized JUCE-facing theme/design tokens, spacing/typography/state language, distinctive control treatment and shell hierarchy are integrated without project-format or audio-engine changes.

### 11.2 — Application shell and transport [DONE]

The shared theme, project/transport hierarchy, live Play/Pause treatment, BPM identity, workspace navigation, concise context and separate transient-status/Master-health layers are integrated. Final closure is recorded in `PHASE11_2_AUDIT.md`; CI #288 passed the required core, legacy X11, Linux JUCE, Windows JUCE and macOS JUCE matrix before PR #88 was merged. Closure PR #89 subsequently passed CI #290 and was merged to `main`.

### 11.3 — Arrangement + Browser workspace [DONE]

The bounded design is recorded in `PHASE11_3_DESIGN.md`. 11.3.1 Arrangement visual hierarchy, 11.3.2 Browser visual hierarchy and 11.3.3 Arrangement/Browser drag/drop integration are integrated. `PHASE11_3_AUDIT.md` records the architecture/realtime/project-compatibility/regression review. PR #93 passed CI #298 before implementation merge; closure PR #94 passed CI #300 and was merged to `main`.

### 11.4 — Mixer + plugin workflow [DESIGNED — IMPLEMENTATION NEXT]

The bounded design is recorded in `PHASE11_4_DESIGN.md`. It productizes existing Track / Bus / Master mixing, routing/sends and plugin racks without changing DSP, routing/PDC, plugin-state architecture or `.flow` v11. Implementation is split into 11.4.1 Mixer visual hierarchy/target state, 11.4.2 routing/sends presentation, 11.4.3 plugin-rack workflow/insert state and 11.4.4 regression/closure audit.

The next implementation PR must contain **only 11.4.1** after this design PR is green and merged.

### 11.5 — Piano Roll / Sequencer / Sampler / Automation

Unify musical editing surfaces, selection language, grids, keyboard/mouse interaction and contextual inspectors.

### 11.6 — Adaptive layout / panels / accessibility

Add bounded resizable/adaptive panels where justified, minimum sizes, keyboard focus behavior, tooltips, accessible visible state and high-DPI validation.

### 11.7 — Export / dialogs / settings / diagnostics productization

Move device/plugin scanning/diagnostic-style controls away from the main creative surface and refine dialogs, export flow, recovery messaging and empty/error states.

### 11.8 — Visual regression and closure

Cross-workflow polish, installed-app golden path, layout smoke/visual regression coverage where practical, Linux/Windows/macOS validation and final architecture/realtime/project-compatibility audit.

## Visual language baseline

### Identity rule — mandatory

FLOWDAW must not look like a generic AI-generated SaaS dashboard, admin panel or marketing landing page. The product should have a recognisable music-software identity of its own. Creative use of colour, gradients, diffusion/glow, depth, asymmetry and distinctive control silhouettes is encouraged when it improves hierarchy and feel. Avoid interchangeable rounded-card UI patterns. Visual personality must never reduce readability, focus visibility, input clarity or professional DAW ergonomics.

Primary creative surfaces: Arrangement, Mixer, Piano Roll, Sequencer, Sampler and Automation.

Secondary/contextual surfaces: Sample Browser, plugin rack / plugin selector, inspector/details and export.

Settings/diagnostics surfaces: audio-device configuration, plugin scan/quarantine maintenance and advanced runtime/health information.

## Current checkpoint

- Phase 10 is closed and its final main-branch CI passed.
- 11.1 is DONE.
- 11.2 is DONE.
- 11.3 is DONE; closure PR #94 passed FLOWDAW CI #300 and is merged.
- 11.4 is designed in `PHASE11_4_DESIGN.md`; no 11.4 production implementation has started.
- `.flow` remains v11 and the realtime contract is unchanged.

Next gate: run the Phase 11.4 design PR CI. If green, merge it; then implement only **11.4.1 Mixer visual hierarchy and target state** on a fresh branch.