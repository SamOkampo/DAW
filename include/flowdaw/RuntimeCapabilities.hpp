#pragma once
#include <string>

namespace flowdaw {
struct RuntimeCapabilities {
    std::string uiBackend;
    bool juceCompiled=false;
    bool audioDeviceManager=false;
    bool externalPluginExecution=false;
    bool pluginEditorHosting=false;
    bool crashRecovery=true;
    bool pluginQuarantine=true;
};
RuntimeCapabilities runtimeCapabilities();
std::string runtimeCapabilitySummary(const RuntimeCapabilities& capabilities);
}
