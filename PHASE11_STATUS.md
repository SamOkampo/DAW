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

The shared theme, project/transport hierarchy, live Play/Pause treatment, BPM identity, workspace navigation, concise context and separate transient-status/Master-health layers are integrated. Final closure is recorded in `PHASE11_2_AUDIT.md`; CI #288 passed the required core, legacy X11, Linux JUCE, Windows JUCE and macOS JUCE matrix before PR #88 was merged.

### 11.3 — Arrangement + Browser workspace [NEXT — DESIGN REQUIRED]

Clarify timeline hierarchy, track headers, clip selection, Browser integration, drag/drop targets, empty states and contextual actions. Before implementation, document a bounded 11.3 design with acceptance criteria, dependencies, tests and explicit exclusions. Preserve existing Arrangement editing semantics, Browser asynchronous preview/import behavior, Undo/Redo and realtime safety.

### 11.4 — Mixer + plugin workflow

Redesign Track/Bus/Master strips, metering, routing/sends and plugin-rack presentation while preserving the existing immutable realtime graph and PDC.

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
- 11.2 is DONE; final implementation passed CI #288 and PR #88 was squash-merged to `main` as `8595508b9003d6b8bb7fcf77e37b8aca2e9eb8cd`.
- `PHASE11_2_AUDIT.md` records PASS for architecture, realtime safety, `.flow` v11 compatibility, interaction hierarchy, visual identity and multiplatform CI.
- No 11.3 implementation has started.
- `.flow` remains v11 and the realtime contract is unchanged.

Next step: design/document only the bounded 11.3 Arrangement + Browser workspace subpoint before implementation.