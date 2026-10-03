# Phase 11.7 — Export / dialogs / settings / diagnostics productization

## Purpose

Phase 11.7 productizes existing delivery, configuration, recovery and diagnostic workflows without changing FLOWDAW's DSP, realtime graph, project schema or plugin-state model. The goal is to move technical/maintenance controls away from the main creative surface and make export, settings and failure/recovery states understandable from the production JUCE application.

This phase is UI/control-thread work around capabilities that already exist. It must not be used to claim new mastering quality, standards compliance or plugin sandboxing.

## Architectural invariants

- `.flow` remains project format v11. No Phase 11.7 UI preference may be serialized into Project.
- Project remains the editable source of truth; UI components do not own DSP state.
- The audio callback remains free of filesystem access, locks, logging, UI calls, plugin scanning, waits and new dynamic allocation.
- Export/render continues through the production offline render/PluginHost/routing graph rather than a UI-specific signal path.
- Audio-device mutation, plugin discovery/quarantine maintenance, settings persistence, file dialogs and recovery I/O execute only from appropriate control/message/worker contexts.
- Existing plugin opaque state, routing/PDC, Undo/Redo and SessionRecovery ownership are preserved.
- App-local preferences belong to AppSettings or another explicit machine-local settings seam, never the portable project.

## Visual/product rule

Export and settings must read as FLOWDAW music software, not a SaaS dashboard or generic AI-generated card grid. Use the existing semantic theme, compact DAW density, clear grouping, restrained depth/glow and distinctive controls where useful. Technical detail is progressive-disclosure content; the main creative workspace must prioritize music creation.

## Existing capability boundary

11.7 wraps and clarifies existing production capabilities including master/stem WAV export, JUCE audio-device selection, AppSettings, plugin scanning/safety state, SessionRecovery and runtime status. It may add UI-only presentation models and deterministic validation seams. It must not silently replace those underlying services.

## Implementation sequence

### 11.7.1 — Export flow and delivery feedback

Replace the top-level one-click/file-chooser feel with a bounded export workflow that makes the delivery target explicit while preserving the existing render engine.

Acceptance criteria:

1. Master Mix and Stems are presented as clear delivery modes from one coherent export entry point.
2. Destination selection occurs before rendering and the user can cancel without mutating Project.
3. The UI presents project/render context that is already authoritative (for example project identity and existing render format/rate facts) without inventing unsupported mastering options.
4. Render start/progress/completion/failure states are explicit; failure text identifies the actionable cause where the backend exposes it.
5. Export work does not execute in the audio callback and does not create a second DSP/render path.
6. Existing master/stem export tests and realtime/offline parity coverage remain green; add focused presentation/state tests where deterministic.

Exclusions: new codecs, loudness normalization, dithering modes, mastering presets, cloud delivery and background render architecture.

### 11.7.2 — Settings and Audio I/O information architecture

Move device/configuration controls into a dedicated settings surface instead of occupying the creative workspace.

Acceptance criteria:

1. Audio device/sample-rate/buffer configuration is reachable from a clearly named Settings/Audio surface.
2. App-local settings remain in AppSettings; no device/UI preference enters `.flow`.
3. Invalid/unavailable device states produce a useful message and leave the application recoverable.
4. Apply/cancel semantics are explicit where a change can fail or restart a device.
5. Settings UI is keyboard/focus accessible and obeys Phase 11.6 bounded geometry.
6. Existing settings migration/sanitization and device-path tests remain green.

Exclusions: new audio backend, aggregate-device implementation, project-level device persistence.

### 11.7.3 — Plugin maintenance and diagnostics relocation

Separate creative plugin selection/rack use from plugin discovery, quarantine/safety maintenance and advanced runtime diagnostics.

Acceptance criteria:

1. Scan/maintenance controls no longer compete with primary creative controls in the main workspace.
2. Creative plugin search/filter/insert/editor workflows remain available where producers use them.
3. Scan status and failure/quarantine information are presented in a dedicated maintenance/diagnostics surface.
4. Plugin scanning never occurs in the audio callback and does not weaken PluginSafety/quarantine behavior.
5. Advanced runtime/health details use progressive disclosure and do not become permanent dashboard chrome.
6. Existing VST3/AU fixture, plugin-state, rack, safety and PDC tests remain green.

Exclusions: plugin sandbox process, new plugin formats, marketplace/account UI.

### 11.7.4 — Recovery, empty and error states

Make SessionRecovery and common empty/error states explicit and actionable without changing persistence semantics.

Acceptance criteria:

1. Startup recovery clearly distinguishes recovered autosave, normal last-session restore and a fresh project.
2. Recovery messaging offers safe next actions and never deletes recovery data merely because a dialog was dismissed.
3. Empty plugin/sample/project contexts explain the next valid action without blocking unrelated work.
4. Import/open/save/export/device/plugin errors are concise, contextual and recoverable; no modal loop or silent failure.
5. Clean Save/New/Open/exit semantics continue to clear or preserve recovery state exactly as SessionRecovery defines.
6. Recovery/project round-trip tests remain green and `.flow` stays v11.

### 11.7.5 — Productization integration regression + closure audit

No feature expansion. Audit the integrated 11.7 diff.

Required closure review:

- export uses the production offline PluginHost/routing graph;
- realtime-safety boundary;
- AppSettings vs Project ownership;
- `.flow` v11 compatibility and migration regression;
- audio-device failure/recovery behavior;
- plugin scan/quarantine/state/PDC preservation;
- SessionRecovery clean/dirty semantics;
- dialog cancellation and error-state recovery;
- keyboard/focus/bounded-layout behavior from 11.6;
- FLOWDAW visual-identity compliance;
- Linux / Windows / macOS required CI;
- production golden-path update for export/settings/diagnostics/recovery.

11.7 may be marked DONE only after this audit and closure PR are green and merged.

## Dependencies

- Phase 11.6 closure on main.
- Existing Export/offline-render path and PluginHost routing graph.
- Existing AppSettings v3 and JUCE AudioDeviceManager integration.
- Existing PluginSafety/plugin scanning infrastructure.
- Existing SessionRecovery service.
- Existing Phase 11 theme/layout/accessibility primitives.

## Test strategy

Automated validation should prefer pure presentation/state seams over hardware-dependent UI automation.

Required automated evidence:

- existing master/stem export and realtime/offline parity tests;
- serialization/migration and safe-save tests;
- AppSettings load/save/migration/sanitize coverage;
- SessionRecovery clean/dirty/restore coverage;
- plugin safety/state/fixture/routing/PDC coverage;
- deterministic dialog/presentation-state tests where introduced;
- Linux, Windows and macOS JUCE production build/install/package matrix.

Manual installed-app validation:

- cancel and complete master/stem export;
- invalid/unwritable export destination;
- unavailable audio device and recovery;
- plugin scan with success plus at least one unavailable/failed case where reproducible;
- recovery startup and clean-exit path;
- keyboard/focus and constrained/high-DPI dialog layout.

Manual checks complement CI and must not be described as automated certification.

## Explicitly out of scope

- project format v12 or any project-schema change;
- new DSP, mastering chain, loudness/true-peak algorithm or professional-mastering certification;
- new export codecs/options not already backed by the render layer;
- plugin sandbox architecture or new plugin formats;
- cloud/account/marketplace features;
- broad redesign of Arrangement, Mixer, Piano Roll, Sequencer, Sampler or Automation;
- final application-wide visual regression/Phase 11 closure, which belongs to 11.8.

## Professional-claim boundary

11.7 can make delivery and maintenance workflows production-friendly. It does not by itself prove mastering-grade DSP, standards-compliant loudness, render fidelity under every plugin, or long-session stability. Those claims remain gated by the final MVP audit.

## Next permitted implementation

After this design PR passes required CI and is merged, create a fresh branch from main and implement **only 11.7.1 — Export flow and delivery feedback**.
