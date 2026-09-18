#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/PluginHost.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace flowdaw;

static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static std::filesystem::path bundleFromBinary(std::filesystem::path p){for(int i=0;i<8&&!p.empty();++i){auto ext=p.extension().string();if(ext==".vst3"||ext==".component")return p;p=p.parent_path();}return{};}
int main(int argc,char**argv){try{
    require(argc>1,"test plugin binary path argument missing");auto bundle=bundleFromBinary(argv[1]);require(!bundle.empty(),"could not locate enclosing plugin bundle");
    std::string error;auto found=scanPluginsWithJuce({bundle.parent_path()},error);require(error.empty(),"JUCE scan returned error");require(!found.empty(),"JUCE did not discover test plugin");
    PluginInstance plugin;plugin.format=found.front().format;plugin.identifier=found.front().identifier;plugin.name=found.front().name;plugin.bypass=false;plugin.enabled=true;plugin.wet=1.0f;
    PluginHost host;host.registerBackend(makeJucePluginBackend());AudioBuffer audio;audio.sampleRate=48000;audio.channels=2;audio.interleaved.assign(256*2,1.0f);
    require(host.process(audio,plugin,error),error.c_str());require(std::abs(audio.interleaved[0]-0.5f)<0.02f,"real plugin processing did not apply fixture gain");require(!plugin.opaqueState.empty(),"real plugin state was not captured");
    AudioBuffer second;second.sampleRate=48000;second.channels=2;second.interleaved.assign(128*2,1.0f);require(host.process(second,plugin,error),error.c_str());require(std::abs(second.interleaved[0]-0.5f)<0.02f,"restored plugin state changed processing");
    std::cout<<"Phase 7 JUCE "<<plugin.format<<" runtime OK: "<<plugin.name<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<"Phase 7 JUCE test failed: "<<e.what()<<"\n";return 1;}}
