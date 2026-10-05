# Phase 12.6 — Packaging / install / FLOW Core discovery

**State: IMPLEMENTED / CI CANDIDATE**

Phase 12.6 closes the runtime-location gap left intentionally by Phase 12.4/12.5. FLOWDAW no longer needs a developer build path to find first-party content in an installed or packaged application.

## Runtime discovery contract

`discoverFlowCoreRoot(executablePath, buildFallback)` validates candidate roots by requiring:

- `flow-core.manifest`
- `PROVENANCE.txt`
- a parseable manifest with library ID `flow.core`
- at least one manifest entry

Candidate order is intentionally installation-first:

1. `<prefix>/share/FLOWDAW/content/flow-core` for Linux/Windows installed and portable layouts.
2. `FLOWDAW.app/Contents/Resources/FLOWDAW/content/flow-core` when the executable lives inside a macOS app bundle.
3. an adjacent `content/flow-core` portable layout for future compatible bundles.
4. the compile-time build root only as a development fallback.

The locator runs on startup/control paths only. It is never called from the audio callback.

## Platform packaging

### Linux

Installed/package layout:

`bin/FLOWDAW`
`share/FLOWDAW/content/flow-core/... `

The TGZ CI opens the archive and verifies the manifest plus exactly 12 WAVs and 10 `.flowpreset` files.

### Windows

Installed/package layout:

`bin/FLOWDAW.exe`
`share/FLOWDAW/content/flow-core/... `

The ZIP CI expands the generated archive and verifies manifest presence plus the expected native asset/preset counts.

### macOS

The first-party library is installed inside the application bundle:

`FLOWDAW.app/Contents/Resources/FLOWDAW/content/flow-core/... `

This keeps the DMG self-contained. CI verifies both the installed `.app` and a mounted generated DMG.

## Development fallback

The JUCE target still receives the generated build-tree FLOW Core location through `FLOWDAW_CORE_LIBRARY_BUILD_ROOT`. Runtime discovery uses this only if no valid installed/portable candidate exists, which keeps development builds convenient without baking their absolute path into `.flow` files.

## Tests

`flowdaw_phase12_discovery_tests` covers:

- Linux/Windows prefix discovery;
- macOS app-bundle discovery;
- build fallback behavior;
- invalid/corrupt manifest rejection;
- missing-content behavior.

Normal multiplatform CI additionally compiles the production JUCE runtime and validates the real install/package layouts.

## Compatibility / realtime audit

- Project format remains `.flow` v11.
- Existing `content:<id>` references from Phase 12.5 remain path-independent.
- No project migration is required.
- No content scanning, manifest parsing or filesystem work was added to the realtime audio callback.
