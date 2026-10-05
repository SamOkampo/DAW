# Phase 12.7 — Licensing / content-integrity audit

**Audit state: PASS ON GREEN CI / MERGE**

## Scope

This audit covers the FLOW Core v2 first-party content set introduced by Phase 12:

- 12 deterministic 48 kHz mono WAV assets;
- 10 native `.flowpreset` files;
- `flow-core.manifest`;
- `PROVENANCE.txt`;
- `CONTENT_RIGHTS.txt`;
- Linux/Windows/macOS install and package inclusion.

It does not audit third-party VST3/AU plugins installed by users and does not replace JUCE licensing obligations.

## Provenance evidence

FLOW Core audio is generated deterministically by `flowdaw::writeFlowCoreLibrary` from FLOWDAW source code. The library does not embed third-party recordings or commercial sample-pack audio.

Native presets contain first-party parameter metadata targeting the existing FLOW Keys, FLOW Bass, FLOW 808 and FLOW Lead engines. They do not embed third-party audio.

The generated `PROVENANCE.txt` records every stable content ID and relative asset path. `CONTENT_RIGHTS.txt` records the first-party origin and explicitly states that the notice is **not** a public license grant.

## Automated integrity gates

`auditFlowCoreLibrary` fails when any of the following is detected:

- missing or unparsable manifest;
- wrong library ID;
- missing provenance or rights notice;
- missing manifest-declared asset;
- symlinked bundled content;
- manifest entry absent from provenance;
- WAV decode failure;
- non-48-kHz or non-mono FLOW Core WAV;
- unexpectedly short, silent/out-of-range or non-finite sample data;
- invalid native preset;
- preset ID/category mismatch with manifest;
- undeclared `.wav` or `.flowpreset` file under the library root.

Regression tests additionally tamper with a generated library to prove that missing assets, undeclared WAVs, preset identity mismatch and incomplete provenance fail the audit.

## Package gates

The normal Phase 12 matrix verifies:

- Linux installed tree and TGZ contain the manifest, rights notice, 12 WAVs and 10 presets.
- Windows installed tree and ZIP contain the manifest, rights notice, 12 WAVs and 10 presets.
- macOS installed app bundle and mounted DMG contain the manifest, rights notice, 12 WAVs and 10 presets.

## Repository licensing observation

At the time of this Phase 12.7 audit, the repository has no top-level `LICENSE`, `LICENSE.md` or `COPYING` file.

That fact does **not** introduce a third-party sample dependency into FLOW Core, but it means this audit must not pretend that a public source/content licence has already been selected. `CONTENT_RIGHTS.txt` is therefore provenance evidence, not a licence grant.

Before public source redistribution or final commercial release terms are declared, the product owner should choose the appropriate FLOWDAW distribution licence/EULA and separately satisfy JUCE and any other dependency licences. The README already records that JUCE modules remain subject to JUCE upstream licensing terms.

## Architecture / compatibility

- Project format remains `.flow` v11.
- Stable `content:<id>` references remain unchanged.
- No content audit, manifest parsing, WAV decode or filesystem traversal runs in the audio callback.
- No DSP, routing, PDC or plugin-state behavior changes in 12.7.

## Closure decision

12.7 is technically complete when the full relevant CI matrix is green and the PR merges. The remaining top-level product licence/EULA choice is a release-policy decision, not a hidden claim inside this content audit.
