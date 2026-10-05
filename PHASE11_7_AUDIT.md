# Phase 11.7 Closure Audit — Export / Settings / Diagnostics / Recovery

**State: AUDIT CANDIDATE — 11.7.4 and this closure PR must both pass required CI and merge before Phase 11.7 is DONE.**

This audit closes Phase 11.7 only. It introduces no new DSP, mastering algorithm, plugin format, audio backend, render graph, project schema or realtime behavior.

## Integrated checkpoints

- **11.7.1 — Export flow and delivery feedback.** PR #120 passed FLOWDAW CI and was squash-merged as `cb44b0279e4fccc6400223c6799009b8985462ba`.
- **11.7.2 — Settings and Audio I/O information architecture.** PR #121 passed FLOWDAW CI and was squash-merged as `434c0f968276eb1e247f67adf997272710041b0a`.
- **11.7.3 — Plugin maintenance and diagnostics relocation.** PR #122 passed FLOWDAW CI and was squash-merged as `e4af16fd1a7c266b0625f8c78b862913cca05f93`.
- **11.7.4 — Recovery, empty and error states.** PR #123 is the implementation candidate. It must pass the full FLOWDAW matrix and merge before this closure PR is eligible to merge.

Superseded stacked/preflight history is not closure evidence; merged main-based trees and green required CI are authoritative.

## Export / offline render path

PASS for the integrated 11.7.1 boundary.

- Master Mix and Track Stems share one bounded Export entry point.
- Destination selection occurs before rendering; cancellation leaves Project unchanged.
- Export state explicitly distinguishes destination selection, rendering, completion, failure and cancellation.
- The UI reuses `exportProjectWav` / `exportTrackStems` and the production PluginHost/routing graph. It does not introduce a second UI-only render path.
- Existing Phase 4 export regression and Phase 8 realtime/offline parity coverage remain required.
- 11.7 does not add codecs, loudness normalization, dithering modes or mastering presets.

## Realtime safety

PASS by scoped diff review, subject to the required closure CI.

- Phase 11.7 changes are message/control-thread presentation and maintenance work.
- Audio device changes remain owned by JUCE AudioDeviceManager outside the callback.
- Plugin scanning remains through the existing JUCE discovery path and never moves into the audio callback.
- File choosers, settings persistence, recovery I/O and diagnostic text remain outside realtime processing.
- No 11.7 change adds filesystem I/O, locks, logging, UI calls, plugin scanning/instantiation, waits or new dynamic allocation to `audioDeviceIOCallbackWithContext`.
- AudioEngine graph publication, routing/PDC, external plugin processing and offline/realtime semantics remain unchanged.

## AppSettings vs Project ownership

PASS.

- Audio device names, preferred sample rate, buffer size and other machine-local preferences remain owned by `AppSettings`.
- Plugin quarantine/safety remains machine-local through `PluginSafetyRegistry`.
- Settings/maintenance panel visibility and diagnostic presentation are UI state only.
- No Phase 11.7 preference or diagnostic state is written into Project.

## .flow compatibility

PASS.

- Portable project format remains **v11**.
- Phase 11.7 adds no project field and requires no migration.
- Existing serialization/migration/safe-save coverage remains part of the green core gate.
- Export/settings/plugin-maintenance/recovery presentation does not change persisted musical, routing, automation or plugin opaque state.

## Audio device failure and recovery

PASS for the productization boundary.

- Audio device/sample-rate/buffer controls are reached through a clearly named **Settings / Audio** surface instead of occupying the creative workspace by default.
- Invalid/unavailable setup states provide a recoverable message and direct the user back to Settings / Audio.
- Device changes continue to use JUCE's existing AudioDeviceManager path.
- Device/UI preferences stay machine-local; no project-level device persistence is introduced.
- The bounded utility panel remains subject to Phase 11.6 geometry/focus behavior.

## Plugin maintenance / quarantine / state / PDC

PASS for the integrated 11.7.3 boundary.

- Plugin scanning and validation/quarantine diagnostics are moved out of the primary creative control strip into a hidden-by-default **Plugin Maintenance** surface.
- Creative search/filter/select/editor/rack workflows remain available in the production workspace.
- Validation issue and quarantine summaries are progressive-disclosure diagnostics rather than permanent dashboard chrome.
- Existing `PluginSafetyRegistry` failure threshold, persistence and explicit quarantine clearing semantics are preserved.
- Existing plugin fixture, opaque-state, rack, routing/PDC and realtime/offline tests remain required.
- No sandbox process or new plugin format is claimed.

## Recovery semantics

PASS for the 11.7.4 implementation boundary, subject to PR #123 green CI/merge.

- Startup presentation distinguishes fresh project, restored last session and recovered autosave.
- Loading a recovery snapshot does not consume/delete the only crash snapshot merely because it was presented.
- Save/New/Open continue to use the existing SessionRecovery ownership and reset/clean semantics.
- Failed/cancelled Open, Import and Save paths keep the current project usable and make the next safe action explicit.
- Empty plugin/sample contexts point to a valid next action without blocking unrelated work.
- The new recovery presentation seam is UI-only and does not replace `SessionRecovery`.

## Dialog cancellation and error recovery

PASS for the scoped UI behavior.

- Export cancellation reports that no delivery was written and returns focus to the export control.
- Import/Open/Save cancellation explicitly states that the current project/recovery state is unchanged.
- Import/Open/Save errors include contextual target information where available and keep the app recoverable.
- Device and plugin errors point to Settings / Audio or Plugin Maintenance rather than creating modal loops.
- No user-visible error path in 11.7 silently mutates Project merely to dismiss a dialog.

## Keyboard / focus / bounded layout

PASS by preservation of the Phase 11.6 contract.

- Settings / Audio and Plugin Maintenance are bounded secondary surfaces.
- Hidden utility content does not permanently consume the creative workspace.
- Plugin Maintenance controls receive explicit accessible titles/descriptions and focus order.
- Existing focus-visible, text-entry shortcut isolation and minimum-window contracts remain unchanged.
- Manual installed-app focus/high-DPI checks remain complementary evidence, not automated accessibility certification.

## FLOWDAW visual identity

PASS for Phase 11.7 scope.

The productized export/settings/maintenance surfaces reuse FLOWDAW's dense dark music-software hierarchy, semantic theme, restrained depth and rose/indigo/aqua language. Technical controls are progressively disclosed instead of becoming a generic SaaS/admin dashboard. Primary creative surfaces remain visually dominant.

## Production golden path

`docs/PRODUCTION_GOLDEN_PATH.md` includes a Phase 11.7 installed-app smoke path covering:

- coherent Master Mix / Track Stems export and cancellation/failure feedback;
- Settings / Audio reachability and unavailable-device recovery;
- Plugin Maintenance scan, validation/quarantine diagnostics and preserved creative plugin workflow;
- recovered-autosave vs last-session vs fresh-project messaging;
- safe cancel/error paths for Open/Import/Save;
- empty plugin/sample next actions;
- save/reopen confirmation that 11.7 does not alter `.flow` v11 musical state.

Manual hardware/plugin/error-path checks complement CI and are not represented as automated certification.

## Required closure CI

Before 11.7 may be marked DONE, PR #123 and this closure PR must each satisfy the required FLOWDAW matrix applicable to their scope:

- core-tests;
- legacy-x11-smoke;
- Linux JUCE runtime / VST3 / install / package;
- Windows JUCE runtime / VST3 / install / package;
- macOS JUCE / VST3 / AU / install / DMG.

Any failing required job blocks closure and must be corrected without expanding Phase 11.7 scope.

## Residual boundaries

Phase 11.7 does **not** claim:

- application-wide visual regression or final Phase 11 closure (owned by 11.8);
- new DSP or mastering chain;
- standards-compliant loudness/true-peak mastering certification;
- render fidelity under every third-party plugin;
- long-session stability certification;
- plugin sandbox architecture;
- aggregate-device support;
- project format v12;
- professional mastering readiness.

## Closure decision

**AUDIT CANDIDATE.** Once PR #123 is green and merged, and this closure PR itself passes the full required CI and is merged, **Phase 11.7 is DONE**. The next permitted work is **11.8 — Visual regression and closure**. FLOWDAW must not be described as ready for professional mastering until the later final MVP audit explicitly passes those gates.
