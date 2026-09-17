# Time-stretch backend evaluation

## Phase 2 decision

FLOWDAW Phase 2 keeps the in-tree WSOLA implementation as the **MVP Smart Sampling backend** for pitch-preserving Match BPM inside the explicitly supported ratio range **0.5x–2.0x**.

This is a product-stage decision, not a claim that the current implementation is a mastering-grade replacement for specialized stretch engines.

## Validation contract

The repository now checks two different things:

1. `flowdaw_time_stretch_quality_tests` validates target duration, approximate pitch preservation, transient energy, deterministic output, unsupported-ratio rejection and BPM matching across representative ratios.
2. `flowdaw_stretch_benchmark` processes a reproducible transient-rich synthetic source at 0.5x, 0.75x, 1.0x, 1.25x, 1.5x and 2.0x and prints output duration, processing milliseconds and processing-time / output-time realtime factor.

The benchmark is intentionally informational in CI: hosted runners vary, so Phase 2 does not fail on an arbitrary wall-clock threshold. Invalid output or non-finite benchmark results do fail.

## Why this closes Phase 2

The Smart Sampling workflow now has a deterministic, tested and measurable stretch implementation that can support Match BPM without modifying the original asset. That is sufficient for the functional Phase 2 exit criterion.

## Production follow-up

Before a commercial audio-quality release, evaluate a higher-quality pluggable backend for difficult polyphonic/transient-heavy material and larger creative stretch demands. Candidates should be compared using the same quality and benchmark fixtures so a backend change does not alter project semantics.

The project format stores the source asset and time ratio rather than treating the stretched buffer as irreplaceable source data, so the backend can be upgraded later while preserving non-destructive project behavior.
