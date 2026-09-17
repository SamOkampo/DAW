# FLOWDAW Architecture — Phase 0 decision

## Definitive product stack

**Language:** C++20.

**Desktop/audio framework:** JUCE 9.0.2, pinned by exact tag/commit in the production build.

**UI:** native JUCE component hierarchy for the Studio, timeline, piano roll, mixer and browser. High-density editors should render through JUCE's accelerated/native graphics path and may gain dedicated OpenGL/Metal-backed renderers if profiling justifies it. React/Tauri is not the primary DAW UI because it adds a cross-runtime bridge to the highest-frequency interactions and complicates deterministic state ownership. A web surface remains acceptable later for account/help/marketplace-style screens that do not participate in realtime editing.

**Why JUCE over Rust+CPAL/Tauri:** Rust is memory-safe and CPAL is useful, but a DAW also needs mature Windows/macOS device handling, MIDI, plugin hosting, codecs, file choosers, CoreAudio/WASAPI integration and eventually VST3/AU hosting. JUCE reduces integration risk in those exact areas. We still keep the DSP/domain core isolated so individual algorithms can later be implemented in Rust/C/C++ if there is a measured benefit.

## Thread model

1. **Audio thread** — no filesystem access, no mutex acquisition, no logging, no dynamic allocation, no UI calls. Reads an immutable RenderGraph through one atomic pointer.
2. **UI/control thread** — edits Project state, creates commands for Undo/Redo, publishes new immutable graphs.
3. **Worker pool** — decode/import, waveform generation, BPM/downbeat analysis, transient analysis, cache rendering, project autosave.
4. **Plugin sandbox/scan process (future)** — plugin discovery and crash isolation should not happen inside the primary audio process.

## State ownership

`Project` is the editable source of truth. The realtime engine does not traverse mutable Project containers. The control layer compiles Project into a realtime-friendly immutable `RenderGraph` and atomically publishes it. Old graphs remain alive until it is safe to reclaim them; Phase 0 deliberately retains published graphs for the session, to avoid callback lifetime hazards before the epoch/reclamation system is introduced.

## Module boundaries

- `core/time`: musical time and sample conversion.
- `core/project`: versioned DAW document model.
- `audio/engine`: transport, render graph, mixer, DSP nodes.
- `audio/io`: JUCE device adapter in production.
- `audio/codecs`: import/decode and cache.
- `editing`: non-destructive clip transforms and commands.
- `ui`: Studio/editor components; never owns DSP state.
- `analysis`: waveform, BPM, downbeat/transients.
- `persistence`: project serializer, snapshots/autosave.

## Plugin evolution

VST3/AU support is not part of Phase 0. The mixer/effect model uses stable effect/node IDs so native effects and hosted plugins can occupy the same graph abstraction later. Plugin state must be opaque binary/XML state owned by the plugin adapter, not leaked into the core model.
