#pragma once
#include "flowdaw/PluginHost.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw {
class PluginSafetyRegistry;

// Production JUCE adapter. This header deliberately contains no JUCE types so the
// toolkit-independent core remains usable without JUCE headers/libraries.
std::shared_ptr<IExternalPluginBackend> makeJucePluginBackend();

// Performs a real JUCE scan/description pass for VST3/AU bundles below the supplied
// roots. This may load third-party code, so an optional safety registry can skip
// quarantined bundles and record scan success/failure outside the realtime thread.
std::vector<PluginDescriptor> scanPluginsWithJuce(
    const std::vector<std::filesystem::path>& roots,
    std::string& error,
    PluginSafetyRegistry* safety=nullptr);

} // namespace flowdaw
