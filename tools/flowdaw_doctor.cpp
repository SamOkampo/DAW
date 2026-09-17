#include "flowdaw/AppSettings.hpp"
#include "flowdaw/PluginSafety.hpp"
#include "flowdaw/RuntimeCapabilities.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/SessionRecovery.hpp"
#include <filesystem>
#include <iostream>
#include <string>

using namespace flowdaw;

static void usage(){
    std::cout<<"FLOWDAW Doctor\n"
             <<"  flowdaw-doctor status\n"
             <<"  flowdaw-doctor recover <destination.flow>\n"
             <<"  flowdaw-doctor clear-recovery\n"
             <<"  flowdaw-doctor clear-quarantine <plugin-id>\n";
}

int main(int argc,char**argv){
    try{
        const auto config=defaultSettingsDirectory();
        const auto settingsPath=config/"settings.conf";
        const auto safetyPath=config/"plugin-safety.conf";
        SessionRecovery recovery(config/"recovery");
        PluginSafetyRegistry safety;safety.load(safetyPath);
        const std::string command=argc>1?argv[1]:"status";
        if(command=="status"){
            const auto settings=loadAppSettings(settingsPath);const auto info=recovery.inspect();const auto caps=runtimeCapabilities();
            std::cout<<"FLOWDAW Doctor\nRuntime: "<<runtimeCapabilitySummary(caps)<<"\nConfig: "<<config.string()<<"\nAudio: "<<settings.audio.preferredSampleRate<<" Hz / "<<settings.audio.bufferSize<<" samples\nAutosave: "<<settings.autosaveSeconds<<" s\nRecovery: "<<(info.available?"AVAILABLE":"clean")<<"\n";
            if(info.available)std::cout<<"Original: "<<info.originalProject.string()<<"\nAutosave: "<<info.autosaveProject.string()<<"\nGeneration: "<<info.generation<<"\n";
            int quarantined=0;for(auto const&r:safety.records())if(r.quarantined)++quarantined;std::cout<<"Quarantined plugins: "<<quarantined<<"\n";return 0;
        }
        if(command=="recover"){
            if(argc<3){usage();return 2;}auto info=recovery.inspect();if(!info.available){std::cerr<<"No recoverable FLOWDAW session found\n";return 3;}auto p=recovery.loadRecovered(true);ProjectSerializer::save(p,std::filesystem::path(argv[2]));std::cout<<"Recovered project written to "<<argv[2]<<"\n";return 0;
        }
        if(command=="clear-recovery"){recovery.markCleanExit();std::cout<<"Recovery state cleared\n";return 0;}
        if(command=="clear-quarantine"){
            if(argc<3){usage();return 2;}safety.clearQuarantine(argv[2]);safety.save(safetyPath);std::cout<<"Plugin quarantine cleared for "<<argv[2]<<"\n";return 0;
        }
        usage();return 2;
    }catch(const std::exception&e){std::cerr<<"FLOWDAW Doctor error: "<<e.what()<<"\n";return 1;}
}
