# Phase 13 Design — Native Plugins Suite

**State: AUTHORIZED / IN PROGRESS**

Phase 13 expands FLOWDAW's first-party effect suite so a normal production/mix workflow can be completed without third-party plugins. It builds on the existing backend-neutral `PluginInstance` rack model, JUCE production runtime and immutable realtime render graph.

## Non-negotiable invariants

- Production desktop remains JUCE 9.0.2.
- Portable project format remains `.flow` v11; Phase 13 uses the existing generic plugin parameter/state fields.
- Native processor construction, parameter normalization, coefficient preparation and state allocation happen off the audio callback.
- Realtime processing performs no filesystem access, locking, logging, UI calls, plugin scanning/instantiation or dynamic allocation.
- Offline and realtime paths must use the same native DSP implementation where practical.
- Native effects must remain sample-rate and block-size safe within the supported runtime range.
- Existing FLOW Gain / Soft Clip / Width behavior and existing external VST3/AU hosting must not regress.
- Phase 13 does not make professional mastering/loudness compliance claims; those belong to Phase 14.

## Sequence

### 13.1 — Native DSP foundation + FLOW EQ

Unify first-party effects behind prepared `IPluginProcessor` instances rather than the old stateless per-sample realtime shortcut, and introduce a six-band parametric FLOW EQ.

FLOW EQ parameters:
- six independent bell bands;
- frequency per band: 20 Hz–20 kHz;
- gain per band: -18 dB–+18 dB;
- Q per band: 0.10–12;
- output gain: -18 dB–+18 dB;
- rack wet/dry remains the existing generic plugin wet control.

Acceptance:
- all existing native inserts execute through prepared callback-safe processors;
- native processor state/scratch allocation occurs in prepare/control paths only;
- FLOW EQ default is effectively transparent;
- EQ boost/cut behavior is audible and finite;
- offline/realtime output parity is regression-tested;
- output is block-size independent within tolerance;
- all 19 EQ parameters persist through `.flow` v11 save/reopen;
- rack UI exposes a generic native-insert selector and generic native-parameter selector;
- full core/X11/Linux/Windows/macOS CI green before merge.

### 13.2 — FLOW Compressor

Feed-forward dynamics processor with threshold, ratio, attack, release, knee and makeup/output gain. Gain-reduction state may be metered, but UI visualization must remain read-only from callback-published meter state.

### 13.3 — FLOW Limiter

Low-latency limiter foundation with ceiling/input gain/lookahead/release. Phase 13 provides the native effect and deterministic DSP tests; true-peak/loudness compliance belongs to Phase 14.

### 13.4 — FLOW Saturator

First-party saturation modes with drive, tone and mix. Any oversampling must be preallocated/prepared outside the callback.

### 13.5 — FLOW Reverb

Stereo algorithmic reverb with room/decay/damping/pre-delay/width/mix and bounded preallocated delay storage.

### 13.6 — FLOW Delay

Tempo-capable stereo delay with time/sync division, feedback, filtering and ping-pong behavior. Tempo-derived values are compiled into processor configuration outside the callback.

### 13.7 — FLOW Chorus

Modulated delay chorus with rate/depth/feedback/width/mix. No callback allocation.

### 13.8 — FLOW Gate / Expander

Threshold, range, attack, hold and release with deterministic envelope behavior.

### 13.9 — FLOW Utility

Gain, polarity, channel swap, mono/balance and stereo-width utilities consolidated into one native insert.

### 13.10 — Native plugin presets

Versioned first-party effect presets for the Phase 13 suite, separate from portable project paths and compatible with the existing native content/preset architecture where appropriate.

### 13.11 — Native plugin editing/productization

Finish parameter presentation, labels/ranges, keyboard/focus accessibility, rack workflow and any plugin-specific editor surfaces justified by the DSP.

### 13.12 — DSP / realtime regression audit

Audit denormals, NaN/Inf safety, sample-rate independence, block-size independence, bypass/wet behavior, offline/realtime parity, latency reporting and callback allocation boundaries.

### 13.13 — Phase 13 closure

Installed-app golden path using only FLOWDAW native effects: source/instrument → EQ → compression/saturation → ambience → routing/mix → export. Verify project save/reopen, Linux/Windows/macOS packaging, architecture, realtime safety and `.flow` v11 compatibility before marking Phase 13 DONE.

## Explicit non-goals

- No LUFS compliance, platform loudness targets or mastering-grade true-peak claim in Phase 13.
- No dithering policy in Phase 13.
- No external plugin sandbox process in this phase.
- No project schema bump merely to add first-party effect parameters.
