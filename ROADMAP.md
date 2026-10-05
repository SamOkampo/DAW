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

## Phase 11 — UI/UX & Productization [DONE]

Phase 11 has started after the completed Phase 10 closure audit. It is an interface/productization phase and must preserve the existing audio architecture, realtime contract and project compatibility.

Planned sequence:
1. **11.1 Design system and shell architecture** — semantic theme tokens, spacing, typography, interaction states, component hierarchy and clear placement of primary vs secondary/diagnostic controls.
2. **11.2 Application shell and transport** — implement the shared theme and simplify top-level Studio navigation/transport/project identity.
3. **11.3 Arrangement + Browser workspace** — timeline/track hierarchy, clip states, drag/drop and Browser integration.
4. **11.4 Mixer + plugin workflow** — channel strips, metering, routing/sends and plugin-rack presentation.
5. **11.5 Musical editors** — Piano Roll, Sequencer, Sampler and Automation interaction/visual consistency.
6. **11.6 Adaptive layout + accessibility** — bounded panel resizing, minimum sizes, focus-visible, tooltips and high-DPI behavior.
7. **11.7 Export/settings/diagnostics productization** — dialogs, export flow, recovery/error states and relocation of technical controls from the main creative surface.
8. **11.8 Visual regression and closure** — cross-workflow polish, installed-app golden path, Linux/Windows/macOS layout validation and final architecture/realtime/project-compatibility audit.

**Phase 11 closure:** 11.1–11.8 are complete. Phase 11.8 added deterministic layout regression coverage, an installed/package cross-workflow golden path and the final architecture/realtime/project-compatibility audit recorded in `PHASE11_8_AUDIT.md`. The audit PR passed the required core, legacy X11, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package and macOS JUCE/VST3/AU/install/DMG matrix before merge. Project format remains v11 and the realtime contract is unchanged.

No Post-MVP phase is authorized by this roadmap closure. See `PHASE11_STATUS.md` and `PHASE11_8_AUDIT.md` for the final acceptance evidence.

**Production desktop path remains JUCE 9.0.2. Project format remains v11.**


## Phase 12 — Native Sound Library & Preset System [DONE]

Phase 12 is explicitly authorized after the completed Phase 11 closure. It makes a fresh FLOWDAW installation musically useful with legally redistributable first-party sounds and presets while preserving JUCE 9.0.2, realtime safety and .flow v11 compatibility.

Planned sequence:
1. **12.1 Core Library architecture** — versioned content manifest, stable content IDs, safe relative-path resolution and deterministic core tests.
2. **12.2 FLOW Core drum/sample library** — curated Kicks, Snares/Claps, Hats, Percussion, 808s and FX with verified redistribution provenance.
3. **12.3 Native instrument preset catalog** — first-party presets for supported FLOW native instruments.
4. **12.4 Browser integration** — first-party source, search/categories/tags, preview and favorites without replacing user sample roots.
5. **12.5 Starter kits/templates** — native-content-backed Boom Bap, Trap, Lo-Fi and other approved starter sessions.
6. **12.6 Packaging/install/discovery** — reliable native-content discovery in Linux, Windows and macOS installed/package artifacts.
7. **12.7 Licensing/content-integrity audit** — provenance, manifest integrity, missing/duplicate asset and decode validation.
8. **12.8 Final integration/closure audit** — fresh-install native-content golden path plus architecture, realtime, .flow and multiplatform packaging audit.

**Phase 12 closure:** 12.1–12.8 are complete. The clean merge chain #140 → #142 → #143 → #144 → #145 passed the required core, legacy X11, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package and macOS JUCE/VST3/AU/install/DMG gates before merge. The final golden path validates installed-content discovery, integrity, starter templates, `.flow` save/reopen with stable `content:<id>` references, hydration, audible render and master WAV export. `main` was verified after final merge `81d37499`. See `PHASE12_STATUS.md` and `PHASE12_8_AUDIT.md`.

**Production desktop remains JUCE 9.0.2. Project format remains v11.**


## Phase 13 — Native Plugins Suite [IN PROGRESS]

Phase 13 is explicitly authorized after the completed Phase 12 closure. It expands FLOWDAW's first-party effect rack while preserving JUCE 9.0.2, the immutable realtime graph and portable `.flow` v11 projects.

Planned sequence:
1. **13.1 Native DSP foundation + FLOW EQ** — prepared callback-safe native processors, six-band parametric EQ, generic native insert/parameter rack workflow and offline/realtime/persistence regression coverage.
2. **13.2 FLOW Compressor** — threshold, ratio, attack, release, knee and makeup/output gain.
3. **13.3 FLOW Limiter** — ceiling/input gain/lookahead/release foundation; mastering-grade true-peak compliance remains Phase 14.
4. **13.4 FLOW Saturator** — drive, tone, saturation modes and mix with callback-safe preparation.
5. **13.5 FLOW Reverb** — stereo algorithmic ambience with bounded preallocated state.
6. **13.6 FLOW Delay** — tempo-capable delay, feedback/filtering and ping-pong behavior.
7. **13.7 FLOW Chorus** — rate/depth/feedback/width/mix modulation.
8. **13.8 FLOW Gate / Expander** — deterministic dynamics envelope and range control.
9. **13.9 FLOW Utility** — gain/polarity/mono/channel/balance/width utilities.
10. **13.10 Native plugin presets** — versioned first-party presets for the Phase 13 suite.
11. **13.11 Native plugin editing/productization** — parameter presentation, accessibility and justified plugin-specific editor surfaces.
12. **13.12 DSP/realtime regression audit** — denormal/NaN safety, sample-rate/block-size independence, bypass/wet/latency and offline/realtime parity.
13. **13.13 Final integration/closure audit** — installed-app native-only mix/export golden path plus architecture, realtime, compatibility and multiplatform CI closure.

**Current checkpoint:** 13.1 is integrated; 13.2 (#148) and 13.3 (#149) are active ordered gates; 13.4–13.9 are implemented on the clean 13.3 lineage and await ordered CI. 13.10–13.13 remain prepared downstream.

**Production desktop remains JUCE 9.0.2. Project format remains v11. Phase 14 is not started.**
