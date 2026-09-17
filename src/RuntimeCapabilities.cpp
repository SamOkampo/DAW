#include "flowdaw/RuntimeCapabilities.hpp"
#include <sstream>

namespace flowdaw {
RuntimeCapabilities runtimeCapabilities(){
    RuntimeCapabilities c;
#ifdef FLOWDAW_JUCE_RUNTIME
    c.uiBackend="JUCE 9 production runtime";
    c.juceCompiled=true;
    c.audioDeviceManager=true;
#else
    c.uiBackend="X11 bootstrap";
#endif
#ifdef FLOWDAW_EXTERNAL_PLUGIN_RUNTIME
    c.externalPluginExecution=true;
    c.pluginEditorHosting=true;
#endif
    return c;
}
std::string runtimeCapabilitySummary(const RuntimeCapabilities&c){
    std::ostringstream o;o<<c.uiBackend<<" | JUCE "<<(c.juceCompiled?"ACTIVE":"OFF")<<" | AudioDeviceManager "<<(c.audioDeviceManager?"ACTIVE":"OFF")<<" | external plugins "<<(c.externalPluginExecution?"ACTIVE":"SLOTS ONLY")<<" | editors "<<(c.pluginEditorHosting?"ACTIVE":"OFF")<<" | recovery "<<(c.crashRecovery?"ON":"OFF")<<" | quarantine "<<(c.pluginQuarantine?"ON":"OFF");return o.str();
}
}
