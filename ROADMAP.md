# FLOWDAW Roadmap

## Phase 0 — Foundations

Completed in the bootstrap implementation:

- project/domain model;
- 960-PPQ musical clock;
- audio graph foundation;
- WAV decode;
- timeline + waveform;
- transport state;
- draggable non-destructive audio clip;
- save/open with project format versioning;
- undo/redo;
- automated tests;
- runnable desktop Studio shell.

Productionization items that remain intentionally separate from feature work:

- replace X11/PortAudio bootstrap adapters with pinned JUCE app/device adapters on Windows and macOS;
- native file chooser and desktop lifecycle;
- atomic autosave + recovery;
- device/sample-rate/buffer settings and xrun meter;
- epoch/RCU-style render-graph reclamation;
- high-quality sample-rate converter.

## Phase 1 — Step Sequencer / Groove Engine

1. [DONE] 16-step Drum Rack core.
2. [DONE] Original generated Kick/Snare/Hat starter kit.
3. [DONE] Per-step Velocity, Probability and Microtiming model + UI editor.
4. [DONE] Pattern transport synchronized to the same 960-PPQ clock.
5. [DONE] Swing implemented as event scheduling offsets, never destructive audio edits.
6. [DONE] Deterministic Humanize for timing and velocity.
7. [DONE] Lane Mute/Solo.
8. [DONE] Undo + save/open + render tests for all above.
9. [NEXT] 32/64/custom pattern lengths with a paged sequencer view.
10. [NEXT] Visible draggable Pattern blocks in Arrangement.
11. [NEXT] Per-pad gain/pan UI and broader native drum kit browser.
12. [NEXT] Human Feel macro controls and groove presets.

Exit criterion: create a 90 BPM boom-bap drum loop, manipulate groove/step timing, arrange/move the pattern, save/reopen and hear the identical result.

## Phase 2 — Sampling core

Sampler, Chop Mode, transient suggestions, BPM detection, warp metadata and time stretching. Smart BPM and Smart Chop become first-class only after the timing/sequencing layer is stable.
