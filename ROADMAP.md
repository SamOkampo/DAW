# FLOWDAW Roadmap

## Phase 0 — Foundations [DONE]
Audio engine, transport, BPM, timeline, WAV import/playback, waveform, clip movement, non-destructive project model, save/open and Undo/Redo.

## Phase 1 — Step Sequencer / Groove Engine [DONE]
16/32/64-step patterns, native drums, pattern Arrangement blocks, Velocity/Probability/Microtiming, Swing/Humanize, lane mixer controls, presets, persistence and render tests.

## Phase 2 — Smart Sampling [DONE]

Completed:
1. Sample analysis metadata and confidence.
2. BPM + transient detection.
3. Equal Chop, Auto Chop, Beat Chop and Bar Chop.
4. Non-destructive persistent slices with manual marker editing.
5. Beat-grid / downbeat estimation and waveform overlay with confidence.
6. WSOLA Match BPM baseline with non-destructive derived assets and reopen regeneration.
7. Automated stretch quality/stability tests over the supported 0.5x–2.0x range.
8. Reproducible stretch performance benchmark with realtime-factor reporting.
9. Multi-bank keyboard Chop pads with realtime one-shot preview independent of transport.
10. Persistent per-slice name, Gain, Pan and Choke Group controls.
11. Stop Preview and same-group choke behavior in preview and Arrangement playback.
12. REC CHOPS performance capture into musical Pattern events.
13. Dedicated draggable CHOPS Arrangement lane with hit markers and Undo.
14. Reversible 0–100% Chop Quantize and deterministic Humanize with 1/8, 1/16 and 1/32 grids plus Reset Feel.
15. Project format v7 with backward loading for v1–v6 and complete Smart Sampling state persistence.
16. CI validation of core tests, Smart Sampling tests, stretch benchmark and complete Studio build.

## Phase 3 — Piano Roll / MIDI / Instruments [DONE]

Completed:
1. Persistent `MidiNote` event model with start, duration, pitch and velocity.
2. Project format v8 with backward loading for v1–v7.
3. Snap/grid utilities plus MIDI note naming/frequency and scale membership helpers.
4. Piano Roll with click-to-create, select, delete, drag-to-move and right-edge resize.
5. 1/8, 1/16 and 1/32 MIDI grids plus editable default note length and velocity.
6. Scale highlighting with root emphasis and Major, Minor, Major Pentatonic and Minor Pentatonic modes.
7. Keyboard pitch preview and octave navigation.
8. Native `FLOW Keys`, `FLOW 808`, `FLOW Bass` and `FLOW Lead` instruments.
9. Native envelope/tone controls in the persistent instrument model.
10. Initial musical FX: soft Drive and tempo-synced Delay.
11. Realtime/offline Arrangement rendering of MIDI notes through the native instrument engine.
12. Dedicated `INSTRUMENT` Arrangement track using the existing Pattern placement model.
13. Automated MIDI, scale, tuning, delay, persistence, migration and Arrangement render tests.
14. Complete Studio executable and all seven test suites validated by GitHub Actions.

## Phase 4 — Recording / Automation / Advanced Mixer [DONE]

Completed:
1. Realtime-safe mono audio input capture with all recording storage preallocated before the callback writes.
2. PortAudio input/output open with automatic output-only fallback when no default microphone is available.
3. Input monitoring and deterministic callback-path recording tests.
4. Persistent non-destructive `RecordingTake` model with musical start/length and active-take comp selection.
5. Recorded takes stored as ordinary WAV-backed SampleAssets instead of destructive track audio.
6. Project format v9 with backward loading for v1–v8.
7. Persistent linear automation curves with normalized control points.
8. Track Volume/Pan, Bus Volume/Pan, Send Gain and Master Volume automation targets.
9. Persistent mixer buses with Volume/Pan/Mute/Solo and gain insert support.
10. Track output routing to Master or Bus.
11. Pre/post-fader-capable sends with independent send gain.
12. Mixer Studio view with track selection, fader/pan, Mute/Solo, Arm, Monitor, recording, take switching, routing, sends and bus controls.
13. Automation point writing at the playhead plus visible automation curve feedback.
14. Offline master WAV export with tail allowance.
15. Per-track WAV stem export.
16. Dedicated Phase 4 tests for recording, monitoring, takes, comp selection, automation, routing, buses, sends, migration and export.
17. CI validation of all eight test suites, stretch benchmark and complete Studio executable.

## Phase 5 — Assist / Plugins / Advanced Workflow [DONE]

Completed:
1. Project format v10 with persistent plugin racks on Master, Tracks and Buses plus backward loading for v1–v9.
2. Persistent plugin identity, format, path, enabled/bypass state, wet mix, parameter values and opaque state blob.
3. SDK-neutral `PluginHost` processor/backend interface so external formats can be integrated without coupling the project model to one SDK.
4. Safe VST3/AU bundle discovery that identifies plugin bundles without executing third-party binaries during scanning.
5. Built-in FLOW Gain, FLOW Soft Clip and FLOW Width processors with real audio processing and wet/bypass behavior.
6. Realtime/offline native master plugin processing in the audio engine; track/bus native gain racks are folded into the published render graph.
7. External VST3/AU slots remain explicitly bypassed in the bootstrap Studio until a registered production backend is available; no fake external processing is claimed.
8. Context snapshot generator that summarizes current BPM, tracks, patterns, samples, buses, automation and plugin state for production assistance.
9. Deterministic context-aware production assistant for headroom, hot tracks, mix-bus opportunities, groove, master soft clipping, alternate takes and sample-tempo checks.
10. Assistant edits require explicit Apply and are committed through the existing Undo stack; informational suggestions never mutate the project.
11. Project Health validator for broken routes, invalid sends/sample references, duplicate IDs, unusual BPM and excessive master gain.
12. Searchable advanced-workflow command model with explicit commands for vocal arming, bus creation, solo cleanup and external-plugin bypass.
13. Phase 5 Studio panel with Assist navigation, Apply + Undo, Project Health, plugin scanning, native plugin rack management and command execution.
14. Safer project saving using a temporary file, atomic-style replacement and `.bak` backup of the previous project when one exists.
15. Dedicated Phase 5 tests covering built-in processing, external backend contract/state, discovery, v10 persistence/migration, assistant behavior, project health, workflow commands and engine plugin rendering.
16. CI validation of all nine test suites, the stretch benchmark and the complete Phase 0–5 Studio executable.

## Post-roadmap — Production desktop hardening [NEXT]
Replace the Linux X11 bootstrap with the intended JUCE 9.x desktop shell, register a production VST3 backend (and AU on macOS), add plugin-editor hosting/sandboxing, device/session UX, packaging, crash recovery and release-grade performance profiling without changing the established non-destructive project model.
