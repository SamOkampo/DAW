# FLOW Core Library v1

FLOW Core Library v1 is the first bundled native sample set for FLOWDAW Phase 12.

## Content

The initial library contains twelve deterministic first-party samples:

- 2 kicks
- 1 snare + 1 clap
- 2 hats
- 2 percussion sounds
- 2 808s
- 2 transition FX

The purpose of v1 is to establish a legally clean, testable first-party content pipeline. Library size may grow later without changing stable IDs already shipped.

## Provenance and redistribution

Every WAV is synthesized deterministically by FLOWDAW's own `CoreLibrary` generator. No third-party recordings or commercial sample packs are embedded. The generated install tree includes `PROVENANCE.txt`, which records every stable content ID and generated relative path.

## Build and package behavior

`flowdaw-content-generator` creates the library at 48 kHz during the normal build. CMake installs it under:

`share/FLOWDAW/content/flow-core`

The Phase 12.2 CI checks verify that Linux, Windows and macOS install trees contain the manifest, provenance record and WAV assets. Phase 12.6 will add production runtime discovery/resolution from those installed/package locations.

## Realtime boundary

Library synthesis is a build-time/offline content operation. Manifest parsing, generation, filesystem creation and WAV writing are not part of the audio callback.
