# Audio Engine

## Clock

FLOWDAW uses 960 pulses per quarter note (`PPQ=960`) in 4/4 for Phase 0. Timeline positions are integer ticks. Conversion to samples happens at graph compilation/render boundaries using project BPM and device sample rate.

At constant tempo:

`seconds = ticks / PPQ * 60 / BPM`

`samples = round(seconds * sampleRate)`

Keeping edits in integer musical ticks avoids cumulative floating-point drift when moving clips/patterns on the grid. A future TempoMap will replace the constant-tempo conversion without changing clip positions.

## Realtime rules

The callback must not allocate, lock, touch disk, parse project files, or call UI code. Phase 0 publishes an immutable graph pointer with release/acquire semantics. Audio buffers are decoded before publication and shared by stable ownership.

## Render graph Phase 0

`Transport -> Track Clip Readers -> Track Gain/Pan -> Master Gain -> Output`

Planned evolution:

`Clip Readers / Instruments -> Track FX chain -> sends/buses -> group buses -> Master FX -> limiter/meter -> device`

Automation is compiled to per-block/per-sample parameter ramps rather than querying the UI/project model from the callback.

## Resampling

Phase 0 uses linear interpolation so files with a different sample rate remain playable. This is intentionally replaceable. Before shipping, offline/high-quality and realtime resamplers should be separated and benchmarked; quality-sensitive stretching is a different subsystem from plain sample-rate conversion.

## Smart BPM / time stretch

BPM detection and time stretching do not belong in the realtime callback. Analysis runs on workers, creates metadata/warp information, and the realtime graph consumes precomputed state. Source files remain untouched; transformed audio may be represented by edit metadata or internal cache files.
