# Phase 3 — Piano Roll / MIDI / Native Instruments

**Status: DONE — FLOWDAW can now compose, edit, preview, persist and render pitched musical parts in addition to drums and sampled chops.**

Implemented and tested:

- Persistent `MidiNote` model with stable ID, musical start tick, duration, MIDI pitch and velocity.
- Project format v8 stores MIDI events, native instrument state and Piano Roll musical settings while loading v1–v7 projects with safe defaults.
- MIDI utilities cover pitch-to-frequency conversion, note naming, grid snapping and supported scale membership.
- Piano Roll opens from the Studio with `P` and preserves the existing Step Sequencer / Smart Sampling workflow underneath it.
- Click empty Piano Roll grid space to create notes.
- Click/drag a note to move it in time and pitch.
- Drag the right edge to resize note duration.
- Right-click or Delete/Backspace removes the selected note.
- Arrow-key nudging supports snapped timing and chromatic pitch movement; Shift+Up/Down edits velocity.
- Note velocity and length have visible editor controls.
- MIDI grid cycles across 1/8, 1/16 and 1/32 resolutions.
- Scale root is editable and the Piano Roll highlights root rows plus in-scale rows.
- Supported scale views: Major, Minor, Major Pentatonic and Minor Pentatonic.
- Computer-keyboard pitch preview is available in Piano Roll mode using `A W S E D F T G Y H U J`.
- Octave navigation is available from the UI and with bracket keys.
- Native instrument presets: `FLOW Keys`, `FLOW 808`, `FLOW Bass` and `FLOW Lead`.
- Persistent native instrument model includes gain, pan, attack, release, tone, drive, delay mix and musical delay length.
- Initial native effects include soft Drive and tempo-synced Delay.
- MIDI notes are rendered by the same Arrangement scheduler used by drums and ChopEvents, so Pattern placement/repeat timing remains musical and BPM-aware.
- Offline rendering and the realtime graph share the generated native-instrument audio path.
- Regression discovered during Phase 3 validation around moved `shared_ptr` argument evaluation was fixed by explicitly capturing generated clip length before ownership transfer.
- Dedicated automated Phase 3 tests validate MIDI tuning, note names, snapping, scale membership, native instrument audibility/tuning, tempo delay, Arrangement scheduling, v8 persistence and v7→v8 migration.
- Legacy Phase 0/1/2 tests continue to pass after the v8 schema migration.
- GitHub Actions validates all seven test suites, the stretch benchmark and the complete Studio executable.

## Phase 3 exit decision

The Phase 3 native instruments are intentionally lightweight first-party instruments rather than a replacement for a full plugin ecosystem. Their purpose is to make FLOWDAW musically complete enough to create drums, chops, bass/808 lines and melodic parts inside the application while the future VST3/AU host layer remains a Phase 5 concern.

The Piano Roll now satisfies the Phase 3 composition/editing loop: **create → pitch/time edit → resize → velocity edit → scale guidance → preview → arrange → save/reopen → render**.

**Next milestone: Phase 4 — Recording / Automation / Advanced Mixer.**
