# Phase 13.13 Final Integration / Closure Audit

**State: PRE-CLOSURE — PASS ONLY AFTER ORDERED GREEN MERGES**

## Native-only production golden path

The automated golden path constructs real audio, writes it to WAV, builds a normal `.flow v11` project and runs the production offline render/export path with a first-party-only chain:

Track:
1. FLOW EQ preset
2. FLOW Compressor preset
3. FLOW Saturator preset
4. FLOW Chorus preset
5. FLOW Gate preset
6. FLOW Utility preset

Master:
1. FLOW Delay preset
2. FLOW Reverb preset
3. FLOW Limiter preset

It then verifies:

- project save/reopen with source audio loaded;
- plugin rack order and identity after reopen;
- audible production render with tail;
- finite stereo output;
- bounded sample peaks after limiter;
- `exportProjectWav` output exists;
- exported WAV can be decoded and matches render format/frame count;
- project format remains v11.

## Product workflow

The JUCE production rack exposes:

- native effect type selector;
- Add Native action;
- selected insert wet/dry;
- generic native parameter selector with semantic names/ranges/suffixes;
- versioned compatible native preset selector;
- Apply Preset with Undo and graph republish;
- existing enable/bypass/order/remove behavior.

No native plugin requires an external SDK or third-party binary.

## Closure gates

Phase 13 may be marked DONE only after the ordered implementation lineage is merged with green relevant CI:
1. 13.1 Native DSP foundation + FLOW EQ
2. 13.2 FLOW Compressor
3. 13.3 FLOW Limiter
4. 13.4–13.9 remaining native suite
5. 13.10–13.11 presets/productization
6. 13.12–13.13 audit/golden path

Required final evidence: core tests, legacy X11 smoke, Linux JUCE/VST3/install/package, Windows JUCE/VST3/install/package, macOS JUCE/VST3/AU/install/DMG.

Phase 14 is not started by this audit.
