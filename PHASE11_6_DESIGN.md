# Phase 11.6 Design — Adaptive Layout / Panels / Accessibility

**State: DESIGNED — implementation must start only after this design PR is green and merged.**

Phase 11.6 makes the production JUCE Studio resize predictably, keeps creative surfaces usable across bounded desktop sizes and adds a coherent keyboard/focus/accessibility contract. It is a UI architecture phase. It must not change musical state, DSP, plugin hosting, routing/PDC, offline rendering or the portable project schema.

## Baseline verified on main

- MainComponent currently starts at 1440 × 1040 logical pixels.
- MainWindow owns MainComponent but is not currently configured as a resizable production window.
- The main resized() path uses several fixed-width horizontal strips, a fixed 320 px creative-editor height and a fixed 176 px Mixer deck; the AudioDeviceSelector receives the remaining vertical area.
- The Sample Browser currently occupies min(280, one third of the editor area), but there is no user-adjustable bounded splitter.
- The Automation / Assist surface currently uses fixed left/middle column starts and widths, so it needs a compact layout contract rather than simple window resizability.
- MainComponent and the primary musical editors already opt into keyboard focus in places, but there is no application-wide traversal/focus-visible contract.
- Only a small number of controls currently expose tooltips and there is no dedicated Phase 11 accessibility metadata/handler layer.

These are design inputs, not permission to rewrite the existing UI architecture wholesale.

## Product objective

A producer should be able to resize FLOWDAW without controls silently clipping, creative editors becoming unusable or focus disappearing. Layout changes must preserve the dense desktop-DAW identity established in 11.1–11.5 rather than turning the application into a generic responsive web dashboard.

At every supported size the user must be able to identify:

- the active workspace/editor;
- the primary creative canvas;
- current project/transport state;
- current Mixer target and essential signal controls;
- focused keyboard control/editor;
- whether a secondary panel is expanded, collapsed or unavailable;
- the accessible name/state/value of interactive controls where JUCE exposes them.

## Authoritative state ownership

11.6 must not create project-level UI state.

- Project, Pattern, SampleAsset, AutomationLane, plugin/routing state and Undo/Redo remain authoritative exactly as before.
- Window size, splitter positions, expanded/collapsed utility-panel state and keyboard focus are presentation state only.
- Phase 11.6 does not require any .flow field or project-format migration.
- Layout persistence is not required for the 11.6 MVP. If later persistence is justified, it must be machine-local AppSettings state in a separately scoped compatible change, never portable .flow state.
- Resizing/focus operations must remain on the JUCE message thread and must not publish audio graphs merely because geometry changed.

## Supported layout contract

### Window bounds

- Default launch size remains 1440 × 1040 logical pixels for continuity unless implementation evidence requires a small adjustment.
- Minimum supported production size: **1180 × 720 logical pixels**.
- MainWindow must become resizable with an explicit bounds constrainer so unsupported dimensions are rejected rather than producing clipped undefined layout.
- There is no fixed maximum size; large-window layouts remain bounded by panel maxima and available desktop bounds.

### Width modes

Two deterministic width modes are sufficient for Phase 11.6:

1. **Wide — 1380 px and above.** Preserve the current dense horizontal hierarchy where it remains readable.
2. **Compact — 1180–1379 px.** Reflow shell/context/tool groups into bounded secondary rows or compact group layouts. Controls may not silently disappear. If a secondary utility group is collapsed, an explicit visible affordance must restore it.

Phase 11.6 must not introduce arbitrary breakpoint proliferation.

### Panel bounds

Initial implementation targets:

- Sample Browser width: 220–420 px.
- Active creative editor minimum width: 620 px.
- Creative editor minimum height: 240 px.
- Mixer deck height: 140–260 px.
- Bottom utility/audio-device area: 0–220 px and explicitly collapsible when vertical space is constrained.

Splitter or adaptive-layout math must clamp to these bounds. Dragging one panel must never reduce another primary panel below its minimum.

The bottom AudioDeviceSelector content remains the same Phase 6/9 functionality. 11.6 may make its containing utility region collapsible to satisfy bounded layout, but **relocating or productizing audio-device / plugin-scan / diagnostic controls belongs to 11.7**.

## Layout behavior rules

1. Primary creative content wins space before secondary/diagnostic content.
2. No primary button, combo box, slider or editor may end with negative bounds or extend outside its assigned parent region at supported sizes.
3. Stateful controls must not become invisible merely because the window enters compact mode unless an explicit collapsed group exposes a clear restore action.
4. Text labels may truncate with a useful accessible/tooltip value, but action labels must remain unambiguous.
5. No top-level horizontal scrolling is introduced for the shell. Creative canvases may keep their existing domain-specific scrolling/zoom behavior.
6. Browser/editor and editor/Mixer split behavior must be deterministic and bounded.
7. Automation / Assist must replace its fixed absolute 360/320/716 column assumptions with the same bounded layout model used by the production shell, while preserving Automation, Bus Mixer and Assist ownership semantics.
8. Resizing must not alter project selection, editor target, transport state, automation target, Mixer target, plugin target or Undo history.

## Focus and keyboard contract

- Tab and Shift+Tab traverse only visible/enabled controls in a logical order within the active workspace.
- Hidden/collapsed controls must not remain in the active focus chain.
- Custom creative editors keep their existing keyboard editing shortcuts and must show a visible focus treatment when they own keyboard focus.
- Focus indication must not rely on color alone: use border/outline/shape or another non-color cue in addition to the existing FLOWDAW accent language.
- Opening/closing menus, dialogs, plugin editors or collapsed utility regions must restore focus to a sensible invoking control when practical.
- TextEditor typing must not accidentally trigger musical/global editing commands.
- Existing Ctrl/Cmd shortcuts, Piano Roll keys, Sequencer keys and Sampler pad mappings remain semantically unchanged.

## Tooltips and accessibility contract

Phase 11.6 is not a claim of full assistive-technology certification, but interactive UI must expose a useful semantic baseline.

- Ambiguous or abbreviated controls receive concise tooltips/help text; obvious self-labelled controls do not need redundant tooltip spam.
- Buttons/toggles expose an understandable accessible name and current on/off state.
- Sliders expose a meaningful parameter name plus current value/range context rather than a generic slider identity.
- Combo boxes expose the purpose of the choice, not just the selected text.
- Disabled state must remain visually distinguishable without depending only on reduced color saturation.
- Read-only custom visualizations such as meters/automation graphs should expose concise status summaries through adjacent accessible text or a suitable JUCE accessibility representation rather than making every drawn primitive focusable.
- Focus outline and essential non-text state indicators target at least 3:1 contrast against adjacent surfaces; ordinary small text targets at least 4.5:1 where practical. This is a design target, not an accessibility certification claim.

## High-DPI contract

- Layout uses JUCE logical coordinates; Phase 11.6 must not add manual OS-scale multiplication to component geometry.
- Borders, focus outlines, hit targets and text must remain legible at representative 100%, 125%, 150% and 200% desktop scale factors.
- Custom drawing must avoid assuming one physical pixel equals one logical pixel.
- No screenshot-perfect golden image requirement is introduced. Deterministic geometry/state tests plus platform smoke/manual scale validation are preferred.

## Non-negotiable invariants

- Production desktop path remains JUCE 9.0.2.
- .flow remains v11.
- No DSP algorithm, plugin-host, routing/PDC, rendering/export, persistence or musical-edit semantic change.
- No filesystem access, logging, UI work, plugin scanning/instantiation, waits, locks or new dynamic-allocation work may be introduced into the audio callback.
- Existing Undo/Redo semantics remain unchanged; resizing/focus/layout changes are not project edits.
- Existing Arrangement ↔ editor, Mixer ↔ Rack and project-selection synchronization must not regress.
- FLOWDAW's dark production identity, depth, accent hierarchy and dense music-software ergonomics remain mandatory; responsive behavior must not devolve into generic cards.
- Small scoped PRs and green required CI before merge.

## Dependencies

- 11.1 design system and shell architecture: DONE.
- 11.2 shell/transport: DONE.
- 11.3 Arrangement + Browser: DONE.
- 11.4 Mixer + plugin workflow: DONE.
- 11.5 musical editors: DONE through closure PR #110.
- 11.7 later owns export/settings/diagnostics relocation/productization; 11.6 must not pre-empt that work.
- 11.8 later owns application-wide visual-regression/Phase 11 closure.

## Implementation sequence

### 11.6.1 — Resizable shell + deterministic layout metrics

Scope the top-level window and shell geometry only.

Acceptance criteria:

1. MainWindow is user-resizable with the 1180 × 720 logical minimum.
2. MainComponent no longer depends on a single 1440 × 1040 geometry to keep primary controls reachable.
3. A centralized deterministic layout helper/metrics seam computes wide vs compact shell regions; it owns no Project or AudioEngine state.
4. Header, transport, workspace navigation and contextual/tool strips reflow without silent clipping at supported bounds.
5. Resizing does not publish project edits, alter transport or change current selections/targets.
6. Existing visual identity is preserved.

Verification:

- deterministic geometry tests at 1180×720, 1280×800, 1440×1040 and 1920×1080;
- assert primary bounds are non-negative, inside parent bounds and do not overlap incompatible regions;
- JUCE component smoke at minimum/default/large bounds;
- existing core/JUCE regression matrix.

### 11.6.2 — Bounded Browser / editor / Mixer / utility panels

Add user-adjustable or adaptive bounded panel sizing without persistent project state.

Acceptance criteria:

1. Browser clamps to 220–420 px and editor remains at least 620 px wide.
2. Creative editor remains at least 240 px high and Mixer clamps to 140–260 px when shown.
3. Utility/audio-device area can collapse explicitly when vertical space is needed and can be restored without losing device state.
4. Panel changes do not alter project state or Undo/Redo.
5. Automation / Assist adapts its three functional regions without fixed absolute column assumptions and without changing Automation/Bus/Assist semantics.
6. No 11.7 relocation/productization is implemented.

Verification:

- splitter clamp tests;
- repeated resize/drag stress between min/max bounds;
- workspace switching after panel resize;
- Automation/Bus/Assist compact/wide smoke;
- existing project/audio tests remain green.

### 11.6.3 — Focus-visible + keyboard traversal

Unify focus behavior across shell and active creative surfaces.

Acceptance criteria:

1. Tab/Shift+Tab traverse visible controls in logical workflow order.
2. Piano Roll, Sequencer, Sampler and Automation show visible non-color-only focus state where they accept keyboard input.
3. Collapsed/hidden controls are excluded from focus traversal.
4. Returning from menus/dialogs/plugin editors restores useful focus where practical.
5. Text editors do not leak typing into global/musical edit shortcuts.
6. Existing keyboard shortcuts retain semantics.

Verification:

- keyboard traversal smoke for each workspace;
- text-entry shortcut-isolation test where practical;
- focus-state deterministic tests for presentation helpers;
- Linux/Windows/macOS JUCE smoke.

### 11.6.4 — Tooltips / accessibility metadata / high-DPI validation

Add semantic help and scaling validation without claiming certification.

Acceptance criteria:

1. Ambiguous controls have concise tooltips/help.
2. Important buttons/toggles/sliders/combo boxes expose meaningful accessible names/state/value context through JUCE-supported semantics.
3. Read-only meters/graphs expose concise readable status without excessive focus stops.
4. Disabled and focus states are not color-only.
5. Minimum/default/large layouts remain legible at representative 100/125/150/200% scale validation.
6. Hit targets and focus outlines remain usable after scaling.

Verification:

- component accessibility/name/value smoke where JUCE exposes deterministic inspection;
- manual platform checklist for representative scaling;
- layout geometry tests remain scale-independent in logical coordinates;
- required cross-platform CI.

### 11.6.5 — Adaptive/accessibility integration regression + closure audit

No feature expansion. Audit the integrated 11.6 diff.

Required closure review:

- layout ownership and absence of project UI state;
- realtime-safety boundary;
- .flow v11 compatibility;
- Undo/Redo and selection/target preservation during resize;
- Browser/editor/Mixer/utility splitter bounds;
- keyboard focus/traversal and tooltip/accessibility semantics;
- high-DPI/manual scale evidence;
- FLOWDAW visual-identity compliance;
- Linux / Windows / macOS required CI;
- production golden-path update for resizing/focus/accessibility behavior.

11.6 may be marked DONE only after this audit and closure PR are green and merged.

## Tests and validation boundaries

Implementation should prefer a pure or UI-only deterministic layout seam that can be tested without booting audio hardware.

Required automated validation should include:

- layout-region calculations for minimum/compact/default/large logical bounds;
- splitter min/max clamping and no-negative-bounds assertions;
- visible/hidden focus-chain behavior where testable;
- existing serialization/migration tests proving .flow remains v11-compatible;
- existing MIDI/sequencer/sampler/automation/routing/plugin tests proving UI resizing does not change musical behavior;
- JUCE production build/install/package smoke on Linux, Windows and macOS.

Manual validation remains appropriate for:

- native OS high-DPI scaling behavior;
- screen-reader/assistive-technology smoke where available;
- subjective focus visibility/readability across real displays.

Manual checks complement CI and must not be described as automated certification.

## Explicitly out of scope

- export flow redesign;
- audio-device/plugin-scan/quarantine relocation or settings IA redesign beyond temporary bounded/collapsible layout;
- new project/pattern/sample/automation state;
- layout persistence in .flow;
- new DSP, instruments, effects, mastering chain or metering algorithms;
- plugin sandbox architecture;
- final application-wide visual regression / Phase 11 closure (11.8);
- professional mastering certification.

## Professional-claim boundary

11.6 can make FLOWDAW more robust across desktop sizes and improve keyboard/accessibility semantics. It does not validate mastering DSP, metering standards, render quality or long-session production reliability and cannot be used to declare FLOWDAW ready for professional mastering.

## Next permitted implementation

After this design PR passes required CI and is merged, create a fresh branch from main and implement **only 11.6.1 — Resizable shell + deterministic layout metrics**.
