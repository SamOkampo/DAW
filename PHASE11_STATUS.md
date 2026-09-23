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

Define the reusable visual language and workspace hierarchy before restyling individual production surfaces.

Deliverables:
1. A centralized JUCE-facing theme/design-token layer rather than scattered one-off colours and spacing values.
2. Semantic tokens for background/surface/elevated surface, primary/secondary text, accent, selection, warning/error/success, borders and meters.
3. A spacing scale and minimum interactive hit-area rules.
4. Typography hierarchy for app title, workspace title, section title, labels, metadata and compact controls.
5. Explicit component states: normal, hover, pressed, selected/active, disabled and focus-visible.
6. Consistent corner/border treatment and icon/text-button rules.
7. Shell hierarchy that separates top app/transport, workspace navigation, browser/inspector, central workspace, mixer/detail and secondary diagnostics.
8. Minimum-window and high-DPI behavior documented before adaptive-panel work.
9. No project-format or audio-engine change.

### 11.2 — Application shell and transport [ACTIVE]

The theme foundation, shell hierarchy and live transport-state treatment are merged. One final bounded shell refinement remains before closure: **status/context hierarchy**.

Final bounded subpoint acceptance:
- replace the long tutorial-style workflow sentence in the permanent shell with concise context appropriate to a professional DAW;
- keep transient operation feedback in `status_` and meter health in `meterLabel_`, with clearly different visual priority;
- preserve project identity, transport, BPM and workspace navigation as the dominant shell layers;
- do not relocate plugin/device/diagnostic controls yet; that remains explicitly owned by 11.7;
- no Arrangement, Mixer, Browser or editor-internal redesign;
- no DSP, audio callback, routing, plugin graph, persistence or project-format behavior change;
- `.flow` remains v11;
- required CI matrix must be green before merge.

Tests / verification:
- build and existing core tests;
- JUCE runtime smoke;
- legacy X11 smoke;
- Windows and macOS JUCE jobs;
- code review confirming no audio-callback or serialization diff.

### 11.3 — Arrangement + Browser workspace

Clarify timeline hierarchy, track headers, clip selection, Browser integration, drag/drop targets, empty states and contextual actions.

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
- 11.1 design-system architecture is merged.
- 11.2 theme foundation, shell hierarchy and live transport-state treatment are merged.
- **The final 11.2 status/context implementation is active on `phase11-shell-status-context-impl`.**
- Permanent tutorial copy has been removed from the Studio shell and replaced with concise workspace context: Arrange, Piano Roll, Step Sequencer, Automation or Sampler.
- Transient operation feedback remains in `status_` with stronger visual priority.
- Master meter health is now a compact, separate summary showing max-channel true peak, sample peak and RMS.
- No plugin/device/diagnostic relocation is included; that remains owned by 11.7.
- No Arrangement, Mixer, Browser or editor-internal redesign is included.
- Audio callback, DSP, routing/PDC, plugin graph, persistence and serialization remain unchanged.
- `.flow` remains v11.

Next gate: full required CI matrix. If green, merge this implementation and perform the bounded 11.2 closure audit before entering 11.3.
