#pragma once
#include "flowdaw/PluginHost.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw {

// Production JUCE adapter. This header deliberately contains no JUCE types so the
// toolkit-independent core remains usable without JUCE headers/libraries.
std::shared_ptr<IExternalPluginBackend> makeJucePluginBackend();

// Performs a real JUCE scan/description pass for the VST3/AU bundles found below
// the supplied roots. Unlike Phase 5's path-only discovery this may load plugin
// code and therefore belongs behind explicit user action/quarantine policy.
std::vector<PluginDescriptor> scanPluginsWithJuce(
    const std::vector<std::filesystem::path>& roots,
    std::string& error);

} // namespace flowdaw
