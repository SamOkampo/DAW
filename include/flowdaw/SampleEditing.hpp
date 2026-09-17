#pragma once
#include "flowdaw/Project.hpp"
#include <cstddef>

namespace flowdaw {
// Moves the shared boundary between slices[boundaryIndex] and
// slices[boundaryIndex + 1]. Both slices remain contiguous and at least
// minSliceFrames long. Returns false when the boundary cannot be moved.
bool moveSliceBoundary(SampleAsset& sample,std::size_t boundaryIndex,SampleIndex newFrame,SampleIndex minSliceFrames=64);

// Inserts a non-destructive boundary into the slice containing frame.
// Useful for manual Chop editing. Returns false if frame is too close to an
// existing edge or no slice contains it.
bool insertSliceBoundary(SampleAsset& sample,SampleIndex frame,SampleIndex minSliceFrames=64);

// Removes the shared boundary, merging two adjacent slices.
bool removeSliceBoundary(SampleAsset& sample,std::size_t boundaryIndex);
}
