# Phase 2 — Smart Sampling / Chop Mode

**Status: IN PROGRESS — the core Boom Bap sample workflow is now playable, recordable and non-destructively editable.**

Implemented and tested:

- Sample BPM metadata and confidence.
- Transient analysis.
- Equal Chop and Auto Chop as non-destructive `SampleSlice` ranges.
- BPM detector tested with synthetic click tracks.
- WSOLA-style pitch-preserving time stretch baseline.
- Match BPM creates a derived asset; the original audio is never modified.
- Derived assets persist source asset ID + time ratio and regenerate on project reopen.
- Slices are remapped when Match BPM changes sample duration.
- GUI actions: `ANALYZE`, `CHOP 8`, `AUTO CHOP`, `MATCH BPM`.
- 16-pad Chop Mode mapped to `1 2 3 4 / Q W E R / A S D F / Z X C V`.
- Realtime one-shot preview uses a fixed SPSC command ring + 16 fixed voices and works while transport is stopped.
- REC CHOPS records pad performance as musical `ChopEvent` objects instead of destructively rendering audio.
- Dedicated CHOPS Arrangement block with hit markers, grid-snapped dragging and Undo.
- Project format v6 stores exact `recordedTick` / `recordedVelocity` separately from edited `tick` / `velocity`.
- Chop quantize and deterministic humanize are reversible and rebuilt from the original captured performance, so repeated edits do not accumulate timing drift.
- Current Studio shortcut `Q CHOPS 50%` moves events halfway toward a 1/16 grid rather than forcing robotic full quantization.
- Manual waveform slice editing core supports move / insert / merge. The Studio supports marker drag, Shift+click add and right-click merge.
- Slice-count changes are guarded when recorded ChopEvents reference that sample; moving existing boundaries remains safe because slice IDs stay stable.
- Beat-grid baseline estimates BPM, first-beat phase and a confidence-scored 4/4 downbeat candidate.
- Beat/bar slice generation is tested against an offset synthetic click track with accented downbeats.
- End-to-end validation already proved an 82 BPM WAV could be analyzed, chopped, played from pads and matched to 90 BPM while preserving remapped chops.

Next:

1. Expose `CHOP BEAT` / `CHOP BAR` in the Studio.
2. Draw the detected beat grid/downbeat overlay and confidence over the waveform.
3. Add visible Chop quantize/humanize controls (0–100%, grid selection, reset) around the reversible v6 core.
4. Add pad banks beyond the first 16 chops and per-pad gain/pan/choke controls.
5. Benchmark/upgrade the stretch backend for production quality, transients and broader stretch ratios.
