# Phase 6 — Production Platform / Reliability

**Status: DONE — FLOWDAW now has tested desktop/runtime persistence, crash-recovery infrastructure, plugin quarantine, packaged diagnostics and release smoke validation without contaminating portable song files with machine-local state.**

Implemented and tested:

- Machine-local `AppSettings` separate from `.flow` projects.
- Preferred sample rate, normalized audio buffer size, input/output device names, monitoring default, autosave interval, last-project path and plugin-root persistence.
- Atomic settings writes.
- Dirty-session recovery marker plus `autosave.flow` snapshot and generation metadata.
- Explicit recovery loading/export and clean-exit cleanup.
- Persistent plugin failure/quarantine registry with configurable threshold and manual reset.
- Runtime capability reporting that distinguishes the X11 bootstrap, JUCE toolchain availability and actual external-plugin execution.
- Optional `FLOWDAW_ENABLE_JUCE_RUNTIME` CMake contract; this does not claim a real VST3/AU backend by itself.
- Packaged `flowdaw-doctor` utility with status, recovery and quarantine commands.
- Version 0.6.0 install rules and CPack TGZ generation.
- CI smoke testing of installed Studio + Doctor binaries and distributable package creation.
- Dedicated Phase 6 regression coverage plus all Phase 0–5 suites.

## Project-format decision

Phase 6 deliberately keeps the musical project format at v10. Hardware names, buffer-size preferences, recovery markers and quarantine state are properties of one machine/runtime, not of the song. Keeping those outside `.flow` preserves project portability across computers.

## JUCE / external-plugin boundary

JUCE 9 is now an explicit optional toolchain contract, but the tested desktop shell remains X11 and external VST3/AU binary execution remains disabled until a real backend is implemented and validated. Runtime capability reporting stays honest: JUCE availability alone does not mark external plugins or editor hosting as active.

## Exit decision

Phase 6 is accepted as complete as the reliability/release foundation. The next milestone is implementation work that genuinely changes the shipping runtime rather than adding more contracts around the bootstrap.

**Next milestone: Phase 7 — JUCE Desktop Parity / Real Plugin Runtime.**
