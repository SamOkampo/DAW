# Phase 11.8 Design — Visual Regression and Closure

## Purpose

Phase 11.8 closes Phase 11 by validating the integrated production JUCE UI across workflows and supported desktop platforms. It is a verification/polish phase, not permission to redesign DSP, routing, persistence or plugin architecture.

## Invariants

- Production desktop path remains JUCE 9.0.2.
- Portable project format remains .flow v11.
- No filesystem, locks, logging, UI, plugin scanning/instantiation, waits or new dynamic allocation may be introduced into the audio callback.
- Existing DSP, routing/PDC, automation, plugin state, recovery and realtime/offline semantics remain authoritative.
- FLOWDAW must retain a recognisable premium DAW identity rather than generic SaaS/dashboard styling.
- Small PRs and green scope-appropriate CI before merge.

## 11.8.1 — Visual-regression contract and deterministic layout smoke

Acceptance:
1. Define representative shell/workspace geometry at default, supported minimum and one expanded desktop size.
2. Add deterministic non-pixel-fragile assertions where practical for primary/secondary surface bounds, visibility and hierarchy.
3. Cover Arrangement/Browser, Mixer/plugin workflow, Piano Roll, Sequencer, Sampler, Automation, Export, Settings and Plugin Maintenance.
4. Preserve focus-visible/accessibility and constrained-layout contracts from 11.6.
5. Do not claim screenshot-perfect cross-platform equivalence; native font/raster differences are expected.

## 11.8.2 — Cross-workflow visual polish

Acceptance:
1. Fix only verified hierarchy/readability/state inconsistencies found by 11.8.1.
2. Preserve intentional FLOWDAW rose/indigo/aqua language, depth/glow restraint and dense music-software ergonomics.
3. Avoid interchangeable rounded-card/dashboard patterns.
4. No musical-model or DSP changes.

## 11.8.3 — Installed-app golden path and platform validation

Acceptance:
1. Extend the production golden path with Phase 11 end-to-end visual/workflow checks.
2. Require Linux, Windows and macOS production build/install/package gates.
3. Record manual high-DPI/native-window/plugin-editor checks as complementary evidence, never automated certification.

## 11.8.4 — Final Phase 11 closure audit

Acceptance:
1. Audit architecture and realtime safety.
2. Audit .flow v11 compatibility/migrations and save/reopen regressions.
3. Audit routing/PDC, automation, plugin state, recovery and realtime/offline/export regressions.
4. Audit UI identity, layout/focus/accessibility and end-to-end production flow.
5. Require full CI matrix green before marking Phase 11 DONE.
6. Do not claim professional mastering readiness; that requires the later final MVP audit requested by the roadmap/process.

## Exclusions

No new project schema, DSP/mastering algorithms, plugin formats, sandbox process, audio backend, codecs or Post-MVP features are authorized by 11.8.
