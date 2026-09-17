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

## Phase 4 — Recording / Automation / Advanced Mixer [NEXT]
Vocal/audio recording, takes and comping, automation curves, buses/sends, routing, advanced mixer UX and export workflow.

## Phase 5 — Assist / Plugins / Advanced Workflow
Context-aware production copilot, VST3/AU host layer and workflow polish.
