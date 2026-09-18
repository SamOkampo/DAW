# Phase 7 Status — JUCE Runtime Foundation / Real VST3 Host

Phase 7 is complete when the branch CI for the final head is green and merged to `main`.

## Delivered

- JUCE 9.0.2 is an optional, pinned production-runtime dependency; the toolkit-independent C++20 core remains usable without JUCE.
- `flowdaw-juce` is a real JUCE desktop application target and coexists with the X11 bootstrap while Studio views migrate.
- The JUCE runtime owns device-management/UI responsibilities rather than leaking JUCE types into the project/domain model.
- `JucePluginBackend` implements FLOWDAW's existing `IExternalPluginBackend` contract using JUCE plugin formats.
- VST3 discovery and processor creation are real SDK-backed operations rather than extension-only discovery pretending to execute binaries.
- External processor audio processing and opaque-state capture/restore are implemented through the backend-neutral `PluginInstance` model.
- Plugin editor hosting is available in the JUCE runtime foundation.
- A deterministic JUCE-built VST3 effect fixture is compiled in CI and used to validate discovery, instantiation, processing and state restoration end-to-end.
- The JUCE desktop executable is installed as the product binary `FLOWDAW`; CI smoke-tests that installed artifact.

## Explicit boundaries

- Phase 7 does not replace the X11 Studio yet. The X11 application remains the feature-complete bootstrap while editing views are ported.
- Phase 7 proves a real external-plugin host/backend, but does not yet place arbitrary external processors into every realtime track/bus/master route. That is Phase 8.
- AU host compilation is enabled for Apple builds, but full AU runtime validation is deferred until macOS CI is present.
- Plugin delay compensation is not claimed in Phase 7.
- Cross-platform signing/notarization/installers are not claimed in Phase 7.
- The `.flow` project format stays at v10 because the Phase 5 backend-independent plugin persistence model already stores the required identity, parameter and opaque-state information.

## Regression guarantees

Phase 7 must keep the existing core tests, X11 Studio build, install/package smoke tests and stretch benchmark green. The JUCE job additionally builds the production desktop target plus real VST3 fixture and executes the end-to-end host integration test.

## Next

Phase 8 integrates the real host into the AudioEngine graph with callback-safe lifecycle management, current-topology PDC, realtime/offline route parity, true metering and incremental JUCE Studio migration.
