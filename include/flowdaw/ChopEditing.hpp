#pragma once
#include "flowdaw/Project.hpp"

namespace flowdaw {
// Moves ChopEvents toward the nearest musical grid position without changing
// their sample/slice identity. strength=0 keeps the performance untouched;
// strength=1 snaps fully to the grid.
void quantizeChopEvents(Pattern& pattern,Tick gridTicks,float strength);
}
