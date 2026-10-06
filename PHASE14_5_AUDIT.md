# Phase 14.5 Audit — Mastering Golden Path / Closure

**State: PRE-CLOSURE — PASS ONLY AFTER ORDERED GREEN MERGES**

Golden path:
1. Build a .flow v11 session with FLOW EQ + FLOW Compressor on a track and FLOW Limiter on Master.
2. Save and reopen the project.
3. Render through the production offline graph.
4. Measure programme loudness and true peak.
5. Apply optional -23 LUFS target gain constrained by -1 dBTP.
6. Deliver PCM24 WAV with deterministic TPDF dither.
7. Re-read the delivered WAV and measure it again.
8. Require target-loudness tolerance, true-peak ceiling, non-silence and 24-bit encoding.
9. Require Linux, Windows and macOS production CI/packaging before closure.

Closure invariants:
- JUCE 9.0.2 remains the production desktop path.
- .flow remains v11.
- realtime callback contract is unchanged.
- no external standards certification claim is made.
- no Phase 15 is started by Phase 14 closure.
