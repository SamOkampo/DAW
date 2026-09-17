#pragma once
#include "flowdaw/Project.hpp"

namespace flowdaw {
// Rebuild edited timing/velocity from the original recorded performance.
// This function is intentionally non-cumulative: repeated calls never drift.
void applyChopEditing(Pattern& pattern);

// Moves ChopEvents toward the nearest musical grid position without changing
// their sample/slice identity. strength=0 restores the original performance;
// strength=1 snaps fully to the grid.
void quantizeChopEvents(Pattern& pattern,Tick gridTicks,float strength);

// Adds deterministic timing/velocity looseness on top of quantize. amount=0
// removes the humanize layer and returns to the quantized result exactly.
void humanizeChopEvents(Pattern& pattern,float amount);

// Restores the captured performance exactly and resets editing controls.
void resetChopEditing(Pattern& pattern);
}
