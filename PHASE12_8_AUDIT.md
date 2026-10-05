# Phase 12.8 — Final Native Sound Library integration / closure audit

**State: CLOSED / PASS**

This audit is intentionally stricter than implementation completion. Phase 12 must remain **IN PROGRESS** until the implementation chain is merged into `main` in order, the final main-based closure PR is green, and installed/package evidence is verified.

## Scope and evidence

### Core library architecture

- Versioned `FLOWDAW_CONTENT 1` manifest.
- Stable content IDs.
- Safe relative-path validation; absolute/root/parent traversal is rejected.
- 12 deterministic first-party WAV samples.
- 10 native instrument presets.
- No project-format field was added for installation paths.

### Browser / workflow

- FLOW Core is a distinct first-party Browser source.
- Search covers stable ID, relative path, category and tags.
- Existing background WAV preview/import/drag/favorite/recent paths are reused.
- Favorites/Recent stay in machine-local `AppSettings`.

### Portable projects / starter templates

- Bundled assets use the existing `SampleAsset::nativeKey` field as `content:<stable-id>`.
- Bundled assets keep serialized `SampleAsset::path` empty.
- Blank, Boom Bap, Trap and Lo-Fi templates are built in the core/domain layer.
- Save → reopen without resolver remains structurally loadable.
- Runtime hydration resolves first-party audio after discovery.
- Legacy native-drum keys continue to load.
- Project format remains `.flow` v11.

### Installed/package discovery

- Linux/Windows resolve `<prefix>/share/FLOWDAW/content/flow-core`.
- macOS resolves `FLOWDAW.app/Contents/Resources/FLOWDAW/content/flow-core`.
- Build-tree root is only a development fallback.
- CI inspects TGZ, ZIP and mounted DMG contents.

### Provenance / integrity

- Every manifest entry is expected in `PROVENANCE.txt`.
- `CONTENT_RIGHTS.txt` records first-party origin and explicitly avoids pretending to be a public licence grant.
- Automated audit checks missing/undeclared assets, WAV validity, preset identity/category, symlinks and provenance.
- The repository currently has no top-level public `LICENSE`/`COPYING`; final distribution terms remain a product-owner/release-policy decision and JUCE remains separately licensed.

## Automated golden path

`flowdaw_phase12_golden_path_tests` performs:

generated install layout → runtime discovery → content integrity audit → Boom Bap FLOW Core template → audible offline render → save `.flow` → verify no install prefix serialized → reopen without resolver → hydrate by stable content IDs → audible offline rerender → master WAV export.

This complements, but does not replace, the installed-app manual pass in `docs/PRODUCTION_GOLDEN_PATH.md`.

## Realtime-safety audit

Phase 12 adds filesystem/content operations only to build, startup, Browser worker/control, project-load and offline/audit paths.

Phase 12 does **not** add manifest parsing, recursive scanning, WAV decoding, installation discovery, template construction, project hydration or integrity auditing to `AudioEngine::process` / the external device callback.

The existing immutable realtime graph, PDC, routing and plugin execution model is unchanged.

## DSP / render audit

Phase 12 introduces no new mastering DSP or plugin effect algorithm. Existing render/export code is reused.

The final golden-path regression requires both pre-save and reopened native-content projects to render audible output and requires a non-empty audible master WAV export.

## Compatibility audit

- `.flow` remains version 11.
- v1-v11 loader range remains unchanged.
- Existing synthetic native drum keys retain their legacy load path.
- `content:<id>` is namespaced inside an already-persisted field and is deliberately left unresolved by bare serialization until a first-party content root is supplied.
- Absolute package/build paths are not serialized for bundled template assets.

## Required closure sequence

Phase 12 may be marked DONE only after all of the following are true:

1. 12.3 is recreated/validated cleanly on the current `main`, full relevant CI green, merged, and `main` verified.
2. 12.4 is then recreated/retargeted cleanly on that `main`, green, merged and verified.
3. Repeat in order for 12.5, 12.6 and 12.7.
4. Recreate/retarget 12.8 as a final main-based closure PR containing only the final audit/golden-path delta.
5. Full core, legacy X11, Linux JUCE, Windows JUCE and macOS JUCE/AU/install/package matrix is green on the final closure head.
6. Verify `main` after merge and update `ROADMAP.md` + `PHASE12_STATUS.md` to DONE.
7. Do not start Phase 13 merely because the implementation branches exist; Phase 12 closure must be real first.

## Current decision

**CLOSED / PASS.** The required sequential clean merges completed:

- 12.3 → PR #135 → `82e04034`
- 12.4 → PR #140 → `9edfeee0`
- 12.5 → PR #142 → `3b547aa5`
- 12.6 → PR #143 → `50620e7c`
- 12.7 → PR #144 → `18a4b3ff`
- 12.8 → PR #145 → `81d37499`

The final 12.8 head passed core, legacy X11, Linux JUCE, Windows JUCE and macOS JUCE/AU/install/package validation before merge. `main` was verified after the final merge. Project format remains `.flow` v11 and the realtime-safety contract is unchanged.

Phase 12 does not claim professional mastering readiness and does not start Phase 13.