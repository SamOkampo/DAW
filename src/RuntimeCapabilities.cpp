#include "flowdaw/RuntimeCapabilities.hpp"
#include <sstream>

namespace flowdaw {
RuntimeCapabilities runtimeCapabilities(){
    RuntimeCapabilities c;
#ifdef FLOWDAW_JUCE_RUNTIME
    c.uiBackend="JUCE";c.juceCompiled=true;c.audioDeviceManager=true;c.externalPluginExecution=true;c.pluginEditorHosting=true;
#else
    c.uiBackend="X11 bootstrap";
#endif
    return c;
}
std::string runtimeCapabilitySummary(const RuntimeCapabilities&c){
    std::ostringstream o;o<<c.uiBackend<<" | JUCE "<<(c.juceCompiled?"ON":"OFF")<<" | external plugins "<<(c.externalPluginExecution?"ACTIVE":"SLOTS ONLY")<<" | recovery "<<(c.crashRecovery?"ON":"OFF")<<" | quarantine "<<(c.pluginQuarantine?"ON":"OFF");return o.str();
}
}
