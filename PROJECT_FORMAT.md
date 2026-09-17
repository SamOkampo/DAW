# FLOWDAW Project Format

## Principles

- Non-destructive: source audio is referenced, never rewritten by clip edits.
- Explicit versioning: every file starts with `FLOWDAW_PROJECT <formatVersion>`.
- Stable numeric IDs for assets/tracks/clips/effects.
- Unknown future versions fail explicitly instead of being interpreted incorrectly.
- Autosave/version snapshots will write new files atomically (temp + rename) in the production persistence layer.

## Phase 0 records

The current text format stores:

- name, sample rate, BPM and playhead;
- master gain;
- sample IDs, names and paths;
- track IDs, mixer volume/pan/mute/solo;
- clip IDs, sample reference, start/length ticks, source range, gain and loop flag;
- patterns and their length.

Audio bytes are not embedded. When a project is opened, assets are resolved from their saved paths and decoded into memory. Missing-media relinking is a Phase 1 persistence feature.

## Migration policy

`formatVersion=1` is the first internal format, not a public compatibility promise. Before beta, migrations become explicit `vN -> vN+1` functions and serializer tests must include golden project files.
