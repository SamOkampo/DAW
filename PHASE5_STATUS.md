# Phase 5 — Assist / Plugins / Advanced Workflow

**Status: DONE — FLOWDAW now has a persistent plugin architecture, working native plugin processing, context-aware production assistance, project-health checks and explicit Undoable workflow actions.**

Implemented and tested:

- Project format v10 with backward loading for v1–v9.
- Ordered plugin racks on Master, Tracks and Buses.
- Persistent plugin format, identifier, name, path, enabled/bypass state, wet mix, parameter values and opaque backend state.
- SDK-neutral `IPluginProcessor` / `IExternalPluginBackend` contract.
- Built-in FLOW Gain, FLOW Soft Clip and FLOW Width processors.
- Native master plugin processing in realtime and offline rendering.
- Native track/bus gain plugin folding into the published render graph.
- Safe VST3/AU bundle discovery without executing third-party binaries during scanning.
- External plugin slots default to bypass in the bootstrap Studio until a real backend is registered.
- Deterministic fake external backend test proving processor creation, audio processing and opaque-state roundtrip through the public host contract.
- Context snapshot generation from current project structure and production state.
- Context-aware suggestions for master headroom, hot tracks, mix buses, groove, master soft clipping, alternate takes and tempo-mismatched samples.
- Informational suggestions never mutate the project.
- Mutating assistant actions require explicit Apply and are committed through Undo/Redo.
- Project Health validation for broken track routes, bad sends, missing sample references, duplicate IDs, abnormal BPM and risky master gain.
- Advanced workflow commands for arming/creating a vocal track, creating a mix bus, clearing solos and bypassing external plugin slots.
- Phase 5 Studio view with Assist navigation, Apply + Undo, Project Health, native plugin controls, external discovery/slot creation and workflow command execution.
- Safe project-save replacement with a temporary file and `.bak` recovery copy of the previous project.
- Dedicated Phase 5 regression suite plus all earlier Phase 0–4 suites.
- CI compilation of the complete stacked Studio shell from Phase 0 through Phase 5.

## External plugin boundary

Phase 5 establishes the **host layer and persistent contract**, not a fake binary host. The Linux X11 bootstrap can discover `.vst3` and `.component` bundles and store them as plugin slots, but it deliberately does not load or execute those third-party binaries without a registered production backend.

The next production implementation should register a JUCE 9.x VST3 backend on Windows/macOS/Linux where appropriate, add AU hosting on macOS, plugin editor windows, crash isolation/sandboxing and plugin-validation UX. Because plugin identity/state are already independent of the backend, that work does not require another project-format redesign.

## Exit decision

Phase 5 is accepted as complete because the original architectural goals now exist as tested product behavior: contextual assistance, plugin-host abstraction/persistence, real native plugin processing and advanced workflow tooling. Third-party VST3/AU execution is classified as **production desktop hardening**, where it belongs together with the JUCE migration and plugin sandbox/editor integration.

**Next milestone: production desktop hardening / v1.0 path.**
