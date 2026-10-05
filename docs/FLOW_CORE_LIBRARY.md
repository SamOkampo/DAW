# FLOW Core Library v2

FLOW Core Library v1 is the first bundled native sample set for FLOWDAW Phase 12.

## Content

FLOW Core v2 preserves the twelve deterministic first-party samples introduced in v1:

- 2 kicks
- 1 snare + 1 clap
- 2 hats
- 2 percussion sounds
- 2 808s
- 2 transition FX

FLOW Core v2 also adds ten native-instrument presets:

- 2 Keys presets
- 2 Bass presets
- 2 808 presets
- 2 Lead presets
- 2 Pad-role presets using the existing FLOW Keys engine with long envelopes

The preset catalog uses only native instrument engines already supported by FLOWDAW. It does not introduce new DSP engines or change the portable project schema. Stable IDs from v1 remain unchanged.

## Provenance and redistribution

Every WAV is synthesized deterministically by FLOWDAW's own `CoreLibrary` generator. Native `.flowpreset` files contain first-party parameter metadata only. No third-party recordings or commercial sample packs are embedded. The generated install tree includes `PROVENANCE.txt`, which records every stable content ID and generated relative path.

## Build and package behavior

`flowdaw-content-generator` creates the library at 48 kHz during the normal build. CMake installs it under:

`share/FLOWDAW/content/flow-core`

The Phase 12.2/12.3 CI checks verify that Linux, Windows and macOS install trees contain the manifest, provenance record, twelve WAV assets and ten `.flowpreset` files. Phase 12.6 will add production runtime discovery/resolution from those installed/package locations.

## Realtime boundary

Library synthesis is a build-time/offline content operation. Manifest parsing, generation, filesystem creation and WAV writing are not part of the audio callback.
