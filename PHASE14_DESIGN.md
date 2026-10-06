# Phase 14 Design — Loudness, Dither & Mastering Metering

**State: AUTHORIZED / IN PROGRESS**

Phase 14 adds standards-aligned mastering measurement and delivery while preserving JUCE 9.0.2, the immutable realtime graph and portable `.flow` v11 projects.

Reference targets:
- ITU-R BS.1770-5 for programme loudness and true-peak concepts.
- EBU R 128 / Tech 3341 for LUFS/LRA presentation and EBU-mode style metering.
- FLOWDAW does not claim third-party conformance certification; automated reference/regression tests are the project acceptance gate.

Sequence:
1. **14.1 Offline loudness / true-peak analysis** — K-weighted gated integrated LUFS, momentary/short-term maxima, LRA, sample peak and 4x band-limited true-peak analysis.
2. **14.2 Master export depth + dither policy** — Float32, PCM24 and PCM16 WAV delivery, deterministic TPDF dither when reducing to integer PCM, explicit no-dither option, export report.
3. **14.3 Mastering meter productization** — master LUFS/TP/LRA presentation and export profile controls without audio-thread filesystem or dynamic allocation.
4. **14.4 Realtime/mastering regression audit** — numerical/reference tests, bit-depth/dither statistics, no-NaN/Inf, existing export compatibility and realtime-safety audit.
5. **14.5 Final mastering golden path / closure** — native mastering chain → save/reopen → render → measure → integer-dithered WAV → re-read/verify → multiplatform CI and final audit.

Non-goals:
- no external certification claim;
- no streaming-service-specific normalization guarantee;
- no `.flow` schema bump;
- no Phase 15.
