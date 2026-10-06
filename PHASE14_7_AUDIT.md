# Phase 14.7 Final Mastering Closure Audit

**State: PRE-CLOSURE — PASS ONLY AFTER ORDERED GREEN MERGES**

## Production golden path

The automated Phase 14 golden path performs:

1. Create real stereo source audio.
2. Build a normal `.flow` v11 project.
3. Use first-party EQ / Compressor / Saturator / Limiter processing.
4. Save and reopen the project.
5. Run production offline render.
6. Analyze Integrated LUFS and true peak.
7. Export through the EBU-oriented profile:
   - PCM24
   - TPDF dither
   - -23 LUFS target
   - -1 dBTP ceiling
8. Decode the exported PCM24 WAV.
9. Re-analyze the delivered file.
10. Verify decoded LUFS/true-peak match the export report within quantisation tolerance.
11. Require either target compliance or an explicit true-peak-constrained result.

## Studio product workflow

The production JUCE Export menu retains:

- Master Mix • Float32 WAV
- Track Stems

and adds:

- Analyze Master • LUFS / LRA / dBTP
- EBU R128 Master • PCM24 + TPDF • -23 LUFS / -1 dBTP

Master Analysis is offline/control-thread work and does not replace the callback-safe playback meter.

## Required final evidence

Before Phase 14 can be marked DONE:

- core tests green;
- legacy X11 smoke green;
- Linux JUCE/VST3/install/package green;
- Windows JUCE/VST3/install/package green;
- macOS JUCE/VST3/AU/install/DMG green;
- ordered PR merges verified on `main`;
- ROADMAP / status / audits updated to closed state.

## Claim boundary

A closed Phase 14 means FLOWDAW has an internally validated BS.1770-5 / EBU R128-aligned mastering measurement and delivery workflow.

It does not mean FLOWDAW has received certification or approval from ITU, EBU, a broadcaster, streaming service or independent measurement laboratory.
