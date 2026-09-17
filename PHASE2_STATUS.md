# Phase 2 — Smart Sampling / Chop Mode

**Status: DONE — the Boom Bap sampling workflow is playable, recordable, beat-aware, non-destructive and fully editable in the current Studio.**

Implemented and tested:

- Sample BPM metadata and confidence plus transient analysis.
- Equal Chop, Auto Chop, `CHOP BEAT` and `CHOP BAR` as non-destructive `SampleSlice` ranges.
- Beat-grid estimation with first-beat phase plus confidence-scored 4/4 downbeat candidate.
- Beat/downbeat overlay drawn over the waveform with BPM, beat-grid confidence and downbeat confidence.
- Manual waveform slice editing: drag boundaries, Shift+click insert and right-click merge/remove with identity safeguards.
- Slice names are editable directly from Chop Mode.
- Multi-bank pad workflow: keyboard mapping `1 2 3 4 / Q W E R / A S D F / Z X C V` operates on the selected bank; `[` and `]` switch banks.
- Auto Chop can create up to 64 slices and expose them across pad banks.
- Per-slice Gain, Pan and Choke Group are persistent project data and affect both realtime preview and Arrangement playback.
- Same-choke-group preview voices cut one another without locking the audio callback; recorded Arrangement chops use the same choke semantics.
- Dedicated Stop Preview control.
- Realtime one-shot preview uses a fixed SPSC command ring + fixed voices and works independently of transport.
- REC CHOPS captures pad performances as musical `ChopEvent` objects rather than rendering destructive audio.
- Dedicated CHOPS Arrangement block with hit markers, grid-snapped dragging and Undo.
- Reversible Chop Quantize from 0–100%, deterministic Chop Humanize from 0–100%, selectable 1/8 / 1/16 / 1/32 grid and Reset Feel.
- Original `recordedTick` / `recordedVelocity` remain separate from edited timing/velocity so repeated edits never accumulate drift.
- Project format v7 persists all Smart Sampling state while loading v1–v6 projects with safe defaults.
- WSOLA-style pitch-preserving Match BPM creates a derived asset; the original audio is never modified.
- Derived assets persist source asset ID + time ratio, regenerate on reopen and preserve/remap slice settings.
- Automated WSOLA quality/stability tests cover the supported 0.5x–2.0x range, target duration, approximate pitch preservation, transient energy, deterministic output, unsupported-ratio rejection and BPM matching.
- Reproducible WSOLA benchmark reports processing milliseconds and realtime factor across 0.5x, 0.75x, 1.0x, 1.25x, 1.5x and 2.0x.
- GitHub Actions validates all core tests, the stretch benchmark and the complete Studio executable.

## Phase 2 exit decision

The in-tree WSOLA implementation is accepted as the Phase 2 / MVP Match-BPM backend inside its tested 0.5x–2.0x range. It is measurable and deterministic, but it is not being claimed as a mastering-grade replacement for a specialized commercial stretch engine. A pluggable higher-quality production backend can be evaluated later without changing the non-destructive project model.

See `docs/TIME_STRETCH_EVALUATION.md` for the benchmark/quality contract.

**Next milestone: Phase 3 — Piano Roll / MIDI / Instruments.**
