# FLOWDAW Roadmap

## Phase 0 — Foundations [DONE]
Audio engine, transport, BPM, timeline, WAV import/playback, waveform, clip movement, non-destructive project model, save/open and Undo/Redo.

## Phase 1 — Step Sequencer / Groove Engine [DONE]
16/32/64-step patterns, native drums, pattern Arrangement blocks, Velocity/Probability/Microtiming, Swing/Humanize, lane mixer controls, presets, persistence and render tests.

## Phase 2 — Smart Sampling [IN PROGRESS]

Done:
1. Sample analysis metadata and confidence.
2. BPM detector.
3. Transient detector.
4. Equal Chop and Auto Chop.
5. Non-destructive slice persistence.
6. WSOLA Match BPM baseline.
7. Derived assets + reopen regeneration.
8. Visible slice markers.
9. 16 keyboard-mapped Chop pads.
10. Realtime one-shot preview independent of transport.
11. REC CHOPS performance capture into Pattern events.
12. Project format v6 with original `recordedTick/recordedVelocity` plus reversible edited timing/velocity.
13. 0–100% non-destructive Chop quantize and deterministic Chop humanize core.
14. Dedicated CHOPS Arrangement lane, draggable with grid snap and Undo.
15. Manual slice editing: drag boundary, insert boundary and merge boundary, with identity safeguards.
16. Beat-grid baseline: BPM + first-beat phase + 4/4 downbeat candidate/confidence.
17. Beat/bar slice generator with automated synthetic accent/offset tests.
18. `CHOP BEAT` and `CHOP BAR` exposed in the Studio UI.
19. Automated WSOLA quality/stability suite covering 0.5x–2.0x ratios, duration, approximate pitch preservation, transient energy, deterministic output and BPM matching.
20. CI builds both the complete Studio executable and all core/test targets on Ubuntu.

Next:
21. Show beat-grid/downbeat overlay and confidence in the waveform UI.
22. Add visible Chop quantize/humanize strength controls rather than only the current 50% shortcut.
23. Add pad banks beyond 16 chops, preview stop/choke groups and per-pad gain/pan controls.
24. Add measured performance benchmarking for the stretch backend and evaluate a production-quality backend for difficult transients and extreme-but-supported ratios.
25. Sample workflow polish: slice naming/renaming, bank navigation and contextual pad editing.

## Phase 3 — Piano Roll / MIDI / Instruments
MIDI event model, professional piano roll, scale highlighting, native instruments and initial effects.

## Phase 4 — Recording / Automation / Advanced Mixer
Vocal recording, takes, automation curves, routing and advanced mixer UX.

## Phase 5 — Assist / Plugins / Advanced Workflow
Context-aware production copilot, VST3/AU host layer and workflow polish.
