# FLOWDAW Roadmap

## Phase 0 — Foundations [DONE]
Audio engine, transport, BPM, timeline, WAV import/playback, waveform, clip movement, non-destructive project model, save/open and Undo/Redo.

## Phase 1 — Step Sequencer / Groove Engine [DONE]
16/32/64-step patterns, native drums, pattern Arrangement blocks, Velocity/Probability/Microtiming, Swing/Humanize, lane mixer controls, presets, persistence and render tests.

## Phase 2 — Smart Sampling [DONE]
Smart Sampling, beat/downbeat analysis, non-destructive slicing, Match BPM, Chop pads, REC CHOPS, reversible feel editing and tested persistence/rendering.

## Phase 3 — Piano Roll / MIDI / Instruments [DONE]
Persistent MIDI notes, Piano Roll editing, scale highlighting, native instruments, tempo-aware FX and Arrangement rendering.

## Phase 4 — Recording / Automation / Advanced Mixer [DONE]
Realtime-safe recording, takes, input monitoring, automation, buses, sends, routing, mixer workflow, master export and stems.

## Phase 5 — Assist / Plugins / Advanced Workflow [DONE]
Persistent plugin racks, native processors, SDK-neutral external-host contract, safe VST3/AU discovery, contextual assistant, Project Health, workflow commands and safer project saves.

## Phase 6 — Production Platform / Reliability [DONE]

Completed:
1. Machine-local `AppSettings` model separated from the portable `.flow` project schema.
2. Persistent preferred sample rate, supported buffer size, input/output device names, monitoring default, autosave interval, last project and plugin roots.
3. Atomic settings replacement through temporary-file writes.
4. Crash-session recovery model using a dirty `session.lock` marker plus `autosave.flow` snapshot.
5. Recovery metadata with original project path and generation counter.
6. Clean-exit cleanup and explicit recovery loading/export.
7. Persistent plugin-safety registry with configurable failure threshold and quarantine state.
8. Explicit quarantine reset instead of silently reloading repeatedly failing plugins.
9. Honest runtime capability reporting for bootstrap/JUCE/external-plugin state.
10. Optional JUCE 9 toolchain switch in CMake without claiming a production plugin runtime merely because JUCE is available.
11. `flowdaw-doctor` packaged utility for runtime status, recovery export/cleanup and plugin-quarantine maintenance.
12. FLOWDAW version advanced to 0.6.0 with install rules and CPack TGZ packaging.
13. CI install smoke test for Studio + Doctor and package-generation smoke test.
14. Dedicated Phase 6 tests for settings, buffer normalization, recovery lifecycle, quarantine persistence and runtime capability reporting.
15. CI validation of ten test suites, stretch benchmark, complete Studio, installed binaries and distributable package.
16. Musical project format remains v10 intentionally; local hardware/runtime preferences are not embedded into songs.

## Phase 7 — JUCE Runtime Foundation / Real VST3 Host [DONE]

Completed:
1. Optional JUCE runtime pinned to JUCE 9.0.2 without coupling the domain/audio core to JUCE headers.
2. Real `flowdaw-juce` desktop target alongside the X11 bootstrap.
3. JUCE `AudioDeviceManager`-based runtime/device shell and plugin-editor hosting foundation.
4. Production `IExternalPluginBackend` implementation backed by JUCE audio-plugin formats.
5. Real VST3 discovery and instantiation off the realtime callback.
6. External processor preparation, audio processing and opaque-state capture/restore through the existing backend-neutral plugin contract.
7. Deterministic JUCE-built VST3 fixture used for an end-to-end host integration test.
8. CI validates VST3 discovery → instantiate → process → state roundtrip and the installed JUCE desktop binary.
9. AU host compilation is wired on macOS builds, but AU runtime validation is intentionally deferred until macOS CI exists.
10. X11 remains available until the JUCE Studio reaches editing/workflow parity; Phase 7 does not claim full UI migration.
11. Realtime external-plugin insertion in the project track/bus/master graph is intentionally deferred to Phase 8 rather than faked.
12. Project format remains v10; the real host uses the backend-independent plugin state already established in Phase 5.

## Phase 8 — Realtime Plugin Graph / PDC / JUCE Studio Migration [DONE]

Completed:
1. External VST3/AU processors are created, prepared and state-restored off the audio callback.
2. Immutable prepared Track/Bus/Master plugin chains execute through preallocated realtime resources.
3. Retired plugin graphs/processors are reclaimed safely from the control thread.
4. Persistent bypass, wet/dry and opaque state work across realtime and offline paths.
5. Topology-aware PDC aligns direct, routed and send paths using reported plugin latency.
6. Deterministic latency/reclamation tests cover graph safety and alignment.
7. Real JUCE VST3 fixtures execute through the AudioEngine graph; macOS also validates AU effect and instrument fixtures.
8. Offline bounce, master WAV export and stems use the production PluginHost/routing graph.
9. Track, bus and master routes expose callback-safe sample peak, RMS and true-peak estimates.
10. Core production editing/workflows are available in JUCE: Arrangement, Sequencer, Smart Sampling/REC CHOPS, Piano Roll, recording/takes, mixer/routing/automation, Assist/Health and plugin racks/editors.
11. The X11 shell is retired from the default product path and remains only as an opt-in CI-covered legacy regression/bootstrap target.
12. Linux, Windows and macOS CI validate the production JUCE runtime; Linux packages TGZ, Windows packages ZIP and macOS packages DMG.

**Production desktop path: JUCE 9.0.2. Project format remains v11.**


## Phase 9 — Workflow / Browser / Polish / Autosave [DONE]

Completed:
1. Production JUCE autosave and crash recovery using the existing `SessionRecovery` service.
2. Dirty autosave restore at startup, periodic recovery snapshots and explicit clean-session handling after Save/New/Open.
3. Recovery filesystem work stays on the message/control thread and never enters the realtime audio callback.
4. Sample Browser with persistent local sample roots, bounded recursive WAV discovery and filename/path search.
5. Persistent Favorites and Recent samples through machine-local AppSettings v3, with compatible v1/v2 migration.
6. Browser import actions, favorite toggling and recent-sample tracking.
7. Internal Browser drag source plus workspace drop target using the existing WAV import route; no decoding or filesystem work is added to the audio callback.
8. Project templates for Blank, Boom Bap, Trap and Lo-Fi starting points.
9. First-run onboarding that points new users to templates, the Sample Browser, drag/drop and Commands.
10. Keyboard workflow with a Ctrl/Cmd+K command palette plus shortcuts for New, Open, Save, Import, transport, Undo/Redo and editor views.
11. Final JUCE workflow polish: FLOWDAW Studio hierarchy, dark workspace treatment, active editor-tab state and concise workflow guidance.
12. Linux, Windows and macOS production CI continues to validate the JUCE runtime, real plugin fixtures, packaging and legacy/core regressions.

**Production desktop path remains JUCE 9.0.2. Project format remains v11.**

## Phase 10 — Production Editing / Mixer / Browser / Workflow [DONE]

Completed: production Arrangement, Mixer, Browser, Piano Roll/Sequencer and plugin-workflow polish; focused performance/robustness regression coverage; production golden-path expansion; and a final architecture/realtime/project-compatibility audit. The closure matrix passed core, legacy X11, Linux JUCE, Windows JUCE and macOS JUCE production validation. Project format remains v11 and the realtime-safety contract is preserved.

## Phase 11 — UI/UX & Productization [PLANNED]

Phase 11 is intentionally reserved for the interface itself after Phase 10 is fully closed and audited. It must not begin before the Phase 10 final architecture/realtime/project-compatibility/regression audit is complete.

Planned focus:
1. Establish a cohesive FLOWDAW visual design system: spacing, typography, contrast, component states, iconography and consistent interaction patterns.
2. Replace the current control-dense technical layout with clear production workspaces and stronger hierarchy for Arrangement, Mixer, Browser, Piano Roll, Sequencer, Sampler, Automation and Plugins.
3. Introduce resizable/dockable or otherwise adaptive panels where justified, with sensible minimum sizes and high-DPI behavior.
4. Turn core workflows into visually obvious paths: create/import → arrange → edit → mix → export.
5. Improve channel strips, transport, timeline ruler, inspectors, context menus, dialogs, empty states, selection feedback and drag/drop affordances.
6. Unify keyboard/mouse behavior, focus handling, tooltips and accessibility-visible state across the JUCE application.
7. Reduce exposed implementation/debug-style controls from the primary workspace; move advanced/device/diagnostic controls to appropriate secondary surfaces.
8. Add visual regression/smoke coverage where practical and validate Linux / Windows / macOS layout behavior.
9. Preserve the existing audio architecture and project compatibility; Phase 11 is a product-interface phase, not a DSP rewrite.

**Phase 11 is planning only until Phase 10 is marked DONE.**

