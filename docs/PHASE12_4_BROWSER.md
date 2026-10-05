# Phase 12.4 — FLOW Core Browser integration

FLOW Core is a first-party Sample Browser source, separate from user-added sample folders.

## Behavior

- When a valid FLOW Core manifest/root is supplied, the Browser exposes **FLOW Core** as its first source.
- Only manifest-declared `SAMPLE` entries are shown in the sample list; native instrument presets remain reserved for their instrument workflow.
- Search is case-insensitive across stable content ID, relative path, category and tags.
- FLOW Core rows show their content category instead of a machine-specific absolute directory.
- Existing preview, auto-preview, import, drag/drop, Recent and Favorites paths are reused.
- Favorites/Recent remain machine-local `AppSettings` data. No bundled-content path is written into the portable `.flow` project merely by browsing/favoriting.
- Preview decoding remains on the existing bounded background pool; no manifest parsing, scanning or filesystem work is added to the audio callback.

## Current root contract

Phase 12.4 wires the production JUCE Browser to the generated FLOW Core build root so the integration is exercised by normal multiplatform compilation and development builds.

This is intentionally **not** the final installed-app locator. Phase 12.6 owns robust Linux/Windows/macOS installed/package discovery so release binaries do not depend on a developer build path.

## Verification

Core tests cover manifest metadata search by tag/category and ignore missing content files. The normal Linux/Windows/macOS JUCE matrix compiles the production Browser integration while Phase 12.2/12.3 package checks continue to validate the content artifacts themselves.
