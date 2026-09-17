# FLOWDAW audio engine

## Realtime contract

The shipping architecture remains C++20 with a JUCE 9.x desktop/device adapter. UI state and file analysis never run inside the realtime callback.

The control thread compiles project state into immutable render graphs. The audio callback reads the published graph, mixes timeline clips/pattern events and never mutates the Project.

## Musical scheduling

- Internal resolution: 960 PPQ.
- Pattern event timing is derived from project BPM and ticks.
- Swing delays off subdivisions.
- Humanize is deterministic, so reopening/rendering the same project does not randomly change the performance.
- Probability decisions are deterministic per event/repeat.

## Smart sampling

BPM/transient analysis and WSOLA processing occur outside the realtime callback. Match BPM creates a derived cache buffer and stores only the relationship to its source (`sourceSampleId`, `timeRatio`) in the project.

Current WSOLA is a Phase-2 baseline and is tested for requested duration, approximate pitch preservation and 82→90 BPM behavior. Production builds should benchmark CPU/audio quality against mature stretch implementations before locking the final backend.

## Chop preview

Chop Mode does not start the transport and does not create Arrangement clips just to audition a pad.

- UI/control thread enqueues a small `PreviewCommand` into a fixed SPSC ring.
- The callback consumes commands into 16 preallocated preview voices.
- No mutex is taken and no heap allocation is performed in the callback path.
- Audio-buffer lifetime is retained by the control-side engine owner while previews can reference it.
- Preview voices can overlap, allowing finger-drumming/chop performances.

A device-block test exercises the same `process()` mixer path even on CI machines without a physical audio device.
