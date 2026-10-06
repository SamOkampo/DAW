# Phase 13.12 Audit — Native DSP / Realtime Safety

**State: CLOSED / PASS**

This audit covers the Phase 13 first-party effect architecture before final sequential CI/merge closure.

## Audited native effect set

- FLOW EQ
- FLOW Compressor
- FLOW Limiter
- FLOW Saturator
- FLOW Reverb
- FLOW Delay
- FLOW Chorus
- FLOW Gate
- FLOW Utility
- existing FLOW Gain / Soft Clip / Width regression path

## Realtime architecture

PASS by construction and regression coverage:

- All Phase 13 effects are created through `createBuiltinPluginProcessor` from the control/offline path.
- `RealtimePluginChain::prepare` constructs and prepares native processors before callback execution.
- Vector/delay/filter/envelope storage for EQ, Limiter, Saturator tone state, Reverb, Delay and Chorus is allocated/resized only in `prepare()`.
- `processRealtime()` implementations perform bounded arithmetic over prepared state and do not call filesystem, UI, plugin scanning, logging, mutex APIs or dynamic container growth.
- Native wet/dry remains centralized in `RealtimePluginChain`; limiter latency is reported into the existing PDC chain.
- Existing external VST3/AU processor preparation/execution remains on the same backend-neutral interface.

## DSP regression matrix

Automated coverage validates:

- 44.1 / 48 / 96 kHz preparation;
- representative block sizes from 1 through 1024 frames;
- finite output under normal and maximum declared parameters;
- reset/zero stability;
- bypass transparency;
- generic wet=0 dry behavior for zero-latency native processors;
- limiter lookahead latency reporting;
- dedicated EQ / Compressor / Limiter offline-vs-realtime parity;
- dedicated block-size independence for EQ / Compressor / Limiter;
- functional behavior for Saturator / Reverb / Delay / Chorus / Gate / Utility.

## Project compatibility

PASS by design:

- Phase 13 uses the existing `PluginInstance` format/identifier/wet/parameter fields.
- No project field or serializer schema was added.
- `.flow` remains v11.
- Native effect preset application resolves to ordinary persisted builtin plugin state.
- Effect-preset files use an independent versioned `FLOWDAW_EFFECT_PRESET 1` format and do not alter project compatibility.

## Scope boundary

Phase 13 Limiter is a sample-peak limiter foundation. This phase does **not** claim:
- LUFS compliance;
- mastering-grade true-peak compliance;
- dithering policy;
- delivery-platform loudness targets.

Those remain Phase 14 work.


## Closure evidence

PASS.

- PR #146 (13.1) passed the full FLOWDAW CI matrix and merged as `f336269c`.
- PR #148 (13.2) passed the full matrix and merged as `7f41b0d0`.
- PR #149 (13.3) passed the full matrix and merged as `b4b77974`.
- PR #150 (13.4–13.9) passed the full matrix and merged as `02a2e4f1`.
- PR #151 (13.10–13.11) passed the full matrix and merged as `759bf809`.
- PR #152 (13.12–13.13) passed the full matrix and merged as `d2329753`.

The Phase 13 realtime/DSP audit is therefore closed. The scope boundary remains unchanged: LUFS compliance, mastering-grade true-peak compliance and dithering policy belong to Phase 14.
