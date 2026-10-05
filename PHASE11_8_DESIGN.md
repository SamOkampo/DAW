# Phase 11.8 Design — Visual Regression and Phase 11 Closure

**State: IMPLEMENTATION CONTRACT**

Phase 11.8 closes UI/UX & Productization. It does not add musical features, DSP, mastering algorithms, plugin formats, project fields, or a new rendering architecture.

## Invariants

- Production desktop path remains JUCE 9.0.2.
- Portable project format remains .flow v11; no migration is introduced.
- The audio callback gains no filesystem access, locks, logging, UI work, plugin discovery/instantiation, waits, or dynamic-allocation work.
- Project/Undo, routing/PDC, plugin state, recovery and offline/realtime ownership remain authoritative.
- Visual regression evidence complements, and never substitutes for, functional CI and installed-app smoke testing.

## 11.8 decomposition

### 11.8.1 — Deterministic visual/layout regression contract

Extend pure layout coverage around the already-authoritative ShellLayout/PanelLayout metrics. Validate representative minimum, compact, default and wide desktop geometries and assert primary/secondary regions stay bounded, non-negative and non-overlapping where their contracts require it. Keep tests headless and deterministic; do not introduce screenshot pixel tests that vary by OS font/GPU rendering.

Acceptance:
- representative 1180x720, 1280x800, 1440x1040 and wide layouts are covered;
- creative editor keeps priority over bounded Browser/Mixer/utility surfaces;
- constrained states collapse secondary surfaces rather than producing invalid geometry;
- existing focus/accessibility contracts remain covered;
- full required CI green before merge.

### 11.8.2 — Cross-workflow polish and installed-app closure path

Review Arrangement, Browser, Mixer/routing, racks, Piano Roll, Sequencer, Sampler, Automation, Export, Settings/Audio, Plugin Maintenance and recovery/error states as one product. Fix only verified hierarchy/layout inconsistencies. Extend the production golden path with a final Phase 11 cross-workflow pass.

Acceptance:
- FLOWDAW identity remains dense music software, not generic SaaS/card UI;
- primary creative surfaces remain dominant;
- no new persistent state or DSP behavior;
- installed/package paths remain the production validation target;
- full required CI green before merge.

### 11.8.3 — Final Phase 11 architecture/realtime/compatibility audit

Audit the integrated Phase 11 tree for architecture boundaries, realtime safety, .flow v11 compatibility, Undo/Redo, routing/PDC, plugin state, recovery, export reachability, layout/accessibility regression and Linux/Windows/macOS production CI.

Closure requires:
- all 11.1–11.8 checkpoints integrated;
- core, legacy X11, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package, macOS JUCE/VST3/AU/install/DMG green;
- ROADMAP and PHASE11_STATUS updated only after the closure tree passes;
- no claim of professional mastering readiness from Phase 11 alone.

## Visual identity gate

Reject changes that make FLOWDAW resemble an interchangeable AI/SaaS dashboard. Preserve intentional dark music-production hierarchy, semantic colour, restrained gradients/depth/glow, distinctive transport/editor/mixer controls, readable focus and dense professional ergonomics.

## Next permitted work

After this design/contract PR is green and merged, implement 11.8.1 in a fresh main-based branch.
