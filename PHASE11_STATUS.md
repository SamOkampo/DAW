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

### 11.1 — FLOWDAW design system and shell architecture [ACTIVE]

Define the reusable visual language and workspace hierarchy before restyling individual production surfaces.

Deliverables:
1. A centralized JUCE-facing theme/design-token layer rather than scattered one-off colours and spacing values.
2. Semantic tokens for background/surface/elevated surface, primary/secondary text, accent, selection, warning/error/success, borders and meters.
3. A spacing scale and minimum interactive hit-area rules.
4. Typography hierarchy for app title, workspace title, section title, labels, metadata and compact controls.
5. Explicit component states: normal, hover, pressed, selected/active, disabled and focus-visible.
6. Consistent corner/border treatment and icon/text-button rules.
7. Shell hierarchy that separates:
   - top app/transport layer,
   - workspace navigation,
   - browser/inspector side surfaces,
   - central editor/workspace,
   - mixer/detail surfaces,
   - status/secondary diagnostics.
8. Minimum-window and high-DPI behavior documented before adaptive-panel work.
9. No project-format or audio-engine change.

Acceptance:
- Design tokens and shell rules are documented and can be implemented without inventing per-screen styling.
- Primary/secondary/action hierarchy is defined for Arrangement, Mixer, Browser, Piano Roll, Sequencer, Sampler, Automation and Plugins.
- Focus-visible and contrast expectations are explicit.
- Current technical controls have a documented target surface: primary workspace, contextual inspector, settings/device panel or diagnostics.
- This planning block changes documentation only; implementation begins in the next bounded PR.

### 11.2 — Application shell and transport

Implement the shared theme and restructure the top-level Studio shell: transport, project identity, workspace navigation, status and secondary controls.

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

## 11.1 visual language baseline

### Identity rule — mandatory

FLOWDAW must not look like a generic AI-generated SaaS dashboard, admin panel or marketing landing page. The product should have a recognisable music-software identity of its own. Creative use of colour, gradients, diffusion/glow, depth, asymmetry and distinctive control silhouettes is encouraged when it improves hierarchy and feel. Avoid interchangeable rounded-card UI patterns. Visual personality must never reduce readability, focus visibility, input clarity or professional DAW ergonomics.

The first implementation PR should introduce semantic names, not screen-specific magic values.

Suggested semantic roles:
- `canvas` — deepest workspace background.
- `surface` — panels and editor chrome.
- `surfaceRaised` — dialogs, popovers, active inspectors.
- `borderSubtle` / `borderStrong`.
- `textPrimary` / `textSecondary` / `textMuted`.
- `accent` / `accentHover` / `accentPressed`.
- `selection` / `focusRing`.
- `success` / `warning` / `danger`.
- `meterSafe` / `meterHot` / `meterClip`.

Spacing should use a small repeatable scale (for example 4/8/12/16/24/32 logical px) instead of unrelated literals.

Typography should use system-appropriate JUCE fonts and relative hierarchy rather than requiring bundled font assets.

## Workspace hierarchy

Primary creative surfaces:
- Arrangement
- Mixer
- Piano Roll
- Sequencer
- Sampler
- Automation

Secondary/contextual surfaces:
- Sample Browser
- plugin rack / plugin selector
- inspector/details
- export

Settings/diagnostics surfaces:
- audio-device configuration
- plugin scan/quarantine maintenance entry points
- advanced runtime/health information

The main creative workspace should not require users to visually parse device setup, plugin scanning and diagnostic controls during ordinary arrange/edit/mix work.

## Current checkpoint

- Phase 10 is closed and its final main-branch CI passed.
- **11.1 planning/design-system architecture is merged to `main`.**
- **11.2 shell theme foundation is active** on a bounded implementation branch.
- Current implementation introduces centralized semantic theme tokens plus a distinct top-level Studio treatment: asymmetric dark gradient, restrained rose/indigo/teal diffusion, notched shell controls, focus-visible styling and themed workspace selectors.
- Arrangement, Mixer, Browser internals, Piano Roll, Sequencer, Sampler, Automation, DSP and project serialization are intentionally untouched in this block.
- `.flow` remains v11 and the realtime contract is unchanged.

Next step after this implementation PR is green and merged: continue 11.2 by simplifying/restructuring the application shell and transport hierarchy without folding in the separate Arrangement/Mixer redesigns.
