#include "flowdaw/AppSettings.hpp"
#include "flowdaw/PluginSafety.hpp"
#include "flowdaw/RuntimeCapabilities.hpp"
#include "flowdaw/SessionRecovery.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}

int main(){
 try{
    const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase6_platform";std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    AppSettings s=defaultAppSettings();s.audio.preferredSampleRate=96000;s.audio.bufferSize=300;s.audio.inputDevice="Mic A";s.audio.outputDevice="Interface B";s.autosaveSeconds=45;s.restoreLastSession=false;s.lastProjectPath="/music/beat.flow";s.pluginRoots={root/"pluginsA",root/"pluginsB"};s.sampleRoots={root/"samplesA",root/"samplesB"};
    const auto settingsPath=root/"settings.conf";saveAppSettings(s,settingsPath);auto loaded=loadAppSettings(settingsPath);require(loaded.version==2,"settings save/load upgrades to v2");require(loaded.audio.preferredSampleRate==96000,"sample rate setting persists");require(loaded.audio.bufferSize==256,"buffer size is normalized to supported value");require(loaded.audio.inputDevice=="Mic A"&&loaded.audio.outputDevice=="Interface B","device names persist");require(loaded.autosaveSeconds==45&&!loaded.restoreLastSession,"autosave and restore preferences persist");require(loaded.pluginRoots.size()==2,"plugin roots persist");require(loaded.sampleRoots.size()==2&&loaded.sampleRoots[0]==root/"samplesA"&&loaded.sampleRoots[1]==root/"samplesB","sample roots persist in machine-local settings");
    const auto legacyPath=root/"settings-v1.conf";{std::ofstream legacy(legacyPath);legacy<<"FLOWDAW_SETTINGS 1\nSAMPLE_RATE 44100\nBUFFER 512\nINPUT \"Legacy Mic\"\nOUTPUT \"Legacy Out\"\nMONITOR 1\nAUTOSAVE 60\nRESTORE 1\nLAST_PROJECT \"/music/legacy.flow\"\nPLUGIN_ROOTS 1\nPLUGIN_ROOT \"/legacy/plugins\"\nEND\n";}auto migrated=loadAppSettings(legacyPath);require(migrated.version==2,"v1 settings migrate in memory to v2");require(migrated.audio.preferredSampleRate==44100&&migrated.audio.bufferSize==512,"v1 audio settings survive migration");require(migrated.pluginRoots.size()==1&&migrated.sampleRoots.empty(),"v1 plugin roots survive and new sample roots default empty");

    SessionRecovery recovery(root/"recovery");require(!recovery.inspect().available,"fresh session has no recovery");Project p;p.name="Recover Me";p.transport.bpm=87.0;recovery.beginSession("/music/original.flow");require(!recovery.inspect().available,"session marker alone is not recoverable without autosave");recovery.autosave(p,"/music/original.flow");auto info=recovery.inspect();require(info.available&&info.generation==1,"autosave creates recoverable dirty session");require(info.originalProject==std::filesystem::path("/music/original.flow"),"recovery remembers original project path");auto recovered=recovery.loadRecovered(false);require(recovered.name=="Recover Me"&&recovered.transport.bpm==87.0,"recovered project preserves project state");require(recovery.inspect().available,"loading recovery must not consume the only crash snapshot before the user confirms it");
    Project newer=p;newer.name="Recover Latest";newer.transport.bpm=94.0;recovery.autosave(newer,"/music/second.flow");info=recovery.inspect();require(info.available&&info.generation==2,"repeated autosave advances recovery generation");require(info.originalProject==std::filesystem::path("/music/second.flow"),"latest autosave updates original project path");recovered=recovery.loadRecovered(false);require(recovered.name=="Recover Latest"&&recovered.transport.bpm==94.0,"recovery always loads the latest autosave snapshot");
    recovery.markCleanExit();require(!recovery.inspect().available,"clean exit removes recovery state");

    PluginSafetyRegistry safety(3);safety.noteFailure("bad.vst3","crash one");safety.noteFailure("bad.vst3","crash two");require(!safety.isQuarantined("bad.vst3"),"plugin is not quarantined before threshold");safety.noteFailure("bad.vst3","crash three");require(safety.isQuarantined("bad.vst3"),"plugin is quarantined at threshold");const auto safetyPath=root/"plugin-safety.conf";safety.save(safetyPath);PluginSafetyRegistry safetyReloaded;safetyReloaded.load(safetyPath);require(safetyReloaded.isQuarantined("bad.vst3"),"quarantine survives restart");safetyReloaded.clearQuarantine("bad.vst3");require(!safetyReloaded.isQuarantined("bad.vst3"),"quarantine can be explicitly cleared");

    auto caps=runtimeCapabilities();require(caps.crashRecovery&&caps.pluginQuarantine,"production reliability capabilities are enabled");require(!runtimeCapabilitySummary(caps).empty(),"runtime capability summary is available");
    std::filesystem::remove_all(root);std::cout<<"FLOWDAW Phase 6 production platform tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
