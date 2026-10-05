#pragma once
#include "flowdaw/PluginHost.hpp"

namespace flowdaw {

// Constructs one of FLOWDAW's first-party effect processors. Construction and
// prepare() happen on the control/offline thread. Realtime methods must not
// allocate, lock, log, touch files or call UI code.
std::unique_ptr<IPluginProcessor> createBuiltinPluginProcessor(
    const PluginInstance& plugin,
    std::string& error);

} // namespace flowdaw
