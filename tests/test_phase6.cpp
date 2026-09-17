#include "flowdaw/AppSettings.hpp"
#include "flowdaw/PluginSafety.hpp"
#include "flowdaw/RuntimeCapabilities.hpp"
#include "flowdaw/SessionRecovery.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}

int main(){
 try{
    const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase6_platform";std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    AppSettings s=defaultAppSettings();s.audio.preferredSampleRate=96000;s.audio.bufferSize=300;s.audio.inputDevice="Mic A";s.audio.outputDevice="Interface B";s.autosaveSeconds=45;s.restoreLastSession=false;s.lastProjectPath="/music/beat.flow";s.pluginRoots={root/"pluginsA",root/"pluginsB"};
    const auto settingsPath=root/"settings.conf";saveAppSettings(s,settingsPath);auto loaded=loadAppSettings(settingsPath);require(loaded.audio.preferredSampleRate==96000,"sample rate setting persists");require(loaded.audio.bufferSize==256,"buffer size is normalized to supported value");require(loaded.audio.inputDevice=="Mic A"&&loaded.audio.outputDevice=="Interface B","device names persist");require(loaded.autosaveSeconds==45&&!loaded.restoreLastSession,"autosave and restore preferences persist");require(loaded.pluginRoots.size()==2,"plugin roots persist");

    SessionRecovery recovery(root/"recovery");require(!recovery.inspect().available,"fresh session has no recovery");Project p;p.name="Recover Me";p.transport.bpm=87.0;recovery.beginSession("/music/original.flow");require(!recovery.inspect().available,"session marker alone is not recoverable without autosave");recovery.autosave(p,"/music/original.flow");auto info=recovery.inspect();require(info.available&&info.generation==1,"autosave creates recoverable dirty session");require(info.originalProject==std::filesystem::path("/music/original.flow"),"recovery remembers original project path");auto recovered=recovery.loadRecovered(false);require(recovered.name=="Recover Me"&&recovered.transport.bpm==87.0,"recovered project preserves project state");
    recovery.markCleanExit();require(!recovery.inspect().available,"clean exit removes recovery state");

    PluginSafetyRegistry safety(3);safety.noteFailure("bad.vst3","crash one");safety.noteFailure("bad.vst3","crash two");require(!safety.isQuarantined("bad.vst3"),"plugin is not quarantined before threshold");safety.noteFailure("bad.vst3","crash three");require(safety.isQuarantined("bad.vst3"),"plugin is quarantined at threshold");const auto safetyPath=root/"plugin-safety.conf";safety.save(safetyPath);PluginSafetyRegistry safetyReloaded;safetyReloaded.load(safetyPath);require(safetyReloaded.isQuarantined("bad.vst3"),"quarantine survives restart");safetyReloaded.clearQuarantine("bad.vst3");require(!safetyReloaded.isQuarantined("bad.vst3"),"quarantine can be explicitly cleared");

    auto caps=runtimeCapabilities();require(caps.crashRecovery&&caps.pluginQuarantine,"production reliability capabilities are enabled");require(!runtimeCapabilitySummary(caps).empty(),"runtime capability summary is available");
    std::filesystem::remove_all(root);std::cout<<"FLOWDAW Phase 6 production platform tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
