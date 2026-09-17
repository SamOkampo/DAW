#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/PluginHost.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace flowdaw;

static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static std::filesystem::path bundleFromBinary(std::filesystem::path p){for(int i=0;i<8&&!p.empty();++i){if(p.extension()==".vst3")return p;p=p.parent_path();}return{};}
int main(int argc,char**argv){try{
    require(argc>1,"test VST3 binary path argument missing");auto bundle=bundleFromBinary(argv[1]);require(!bundle.empty(),"could not locate enclosing .vst3 bundle");
    std::string error;auto found=scanPluginsWithJuce({bundle},error);require(error.empty(),"JUCE scan returned error");require(!found.empty(),"JUCE did not discover test VST3");
    PluginInstance plugin;plugin.format="vst3";plugin.identifier=found.front().identifier;plugin.name=found.front().name;plugin.bypass=false;plugin.enabled=true;plugin.wet=1.0f;
    PluginHost host;host.registerBackend(makeJucePluginBackend());AudioBuffer audio;audio.sampleRate=48000;audio.channels=2;audio.interleaved.assign(256*2,1.0f);
    require(host.process(audio,plugin,error),error.c_str());require(std::abs(audio.interleaved[0]-0.5f)<0.02f,"real VST3 processing did not apply fixture gain");require(!plugin.opaqueState.empty(),"real VST3 state was not captured");
    AudioBuffer second;second.sampleRate=48000;second.channels=2;second.interleaved.assign(128*2,1.0f);require(host.process(second,plugin,error),error.c_str());require(std::abs(second.interleaved[0]-0.5f)<0.02f,"restored VST3 state changed processing");
    std::cout<<"Phase 7 JUCE VST3 runtime OK: "<<plugin.name<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<"Phase 7 JUCE test failed: "<<e.what()<<"\n";return 1;}}
