# Phase 12 Design — Native Sound Library & Preset System

**State: IMPLEMENTATION CONTRACT**

Phase 12 makes a fresh FLOWDAW installation musically useful without requiring third-party plugins or user-supplied sample folders. It adds redistributable native content and preset discovery while preserving the existing audio engine, realtime contract and portable project semantics.

## Invariants

- Production desktop remains JUCE 9.0.2.
- Portable project format remains .flow v11 unless an explicit compatible migration is separately justified and tested.
- Native-library discovery, manifest parsing, filesystem scanning and WAV loading never run in the audio callback.
- Bundled content is immutable installation content; favorites, recents and additional user roots remain machine-local settings.
- Projects must not depend on absolute installation paths. Native assets are identified by stable content IDs and resolved outside realtime processing.
- No asset may ship without verified redistribution rights and recorded provenance.
- Phase 12 does not claim new mastering, plugin-DSP or professional loudness functionality.

## Sequence

### 12.1 — Core Library architecture

Add a versioned manifest contract for bundled samples and native-instrument presets, stable content IDs, safe relative-path resolution and deterministic parser tests.

Acceptance:
- manifest parser rejects unsupported schema, duplicate IDs, absolute paths and parent traversal;
- sample entries are WAV and native instrument presets use .flowpreset;
- no Project/.flow schema change;
- toolkit-independent core tests cover the contract;
- full FLOWDAW CI green before merge.

### 12.2 — FLOW Core drum/sample library

Ship a deliberately small, curated redistributable library organized into Kicks, Snares/Claps, Hats, Percussion, 808s and FX. Initial target is quality and legal provenance rather than library size.

Acceptance:
- every shipped asset has stable ID, category/tags and provenance record;
- WAV files decode successfully and are non-empty;
- package/install artifacts include the library on Linux, Windows and macOS;
- no unverified third-party sample is committed.

### 12.3 — Native instrument preset catalog

Add versioned presets for existing FLOW native instruments, beginning with Keys/Bass/808/Pad/Lead-oriented musical roles where supported by the current InstrumentState contract.

### 12.4 — Browser integration

Expose FLOW Core as a first-party Browser source with search, categories/tags, preview and favorites while preserving existing user sample roots and bounded background preview behavior.

### 12.5 — Starter kits/templates

Wire curated native sounds/presets into Boom Bap, Trap, Lo-Fi and additional approved starter templates so a fresh install can create audible music immediately.

### 12.6 — Packaging/install/discovery

Make native content discoverable from installed/package artifacts across Linux TGZ, Windows ZIP/install tree and macOS app/DMG without hard-coded developer paths.

### 12.7 — Licensing and content-integrity audit

Audit provenance, redistribution rights, manifest integrity, duplicate IDs, missing assets, decode validity and package presence.

### 12.8 — Phase 12 closure

Run fresh-install golden path: launch → choose native kit/instrument → preview → arrange/edit → save/reopen → export. Audit architecture, realtime safety, .flow compatibility and multiplatform packaging before marking Phase 12 DONE.

## Explicit non-goals

- No scraped/commercial sample packs without redistribution permission.
- No cloud content marketplace or account requirement.
- No audio-thread filesystem lookup.
- No Phase 13 native effects/plugins in this phase.
