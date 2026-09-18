#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/PluginHost.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace flowdaw;

static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static std::filesystem::path bundleFromBinary(std::filesystem::path p){for(int i=0;i<8&&!p.empty();++i){if(p.extension()==".vst3")return p;p=p.parent_path();}return{};}

int main(int argc,char**argv){try{
    require(argc>1,"test VST3 binary path argument missing");auto bundle=bundleFromBinary(argv[1]);require(!bundle.empty(),"could not locate enclosing .vst3 bundle");
    std::string error;auto found=scanPluginsWithJuce({bundle.parent_path()},error);require(error.empty(),"JUCE scan returned error");require(!found.empty(),"JUCE did not discover Phase 8 fixture");
    PluginInstance plugin;plugin.format="vst3";plugin.identifier=found.front().identifier;plugin.name=found.front().name;plugin.enabled=true;plugin.bypass=false;plugin.wet=1.0f;
    auto host=std::make_shared<PluginHost>();host->registerBackend(makeJucePluginBackend());AudioEngine engine;engine.configureExternalDevice(48000,128,true);engine.setPluginHost(host);engine.setInputMonitoring(true);
    Project project;project.master.plugins.push_back(plugin);engine.publish(project);
    AudioBuffer input;input.sampleRate=48000;input.channels=1;input.interleaved.assign(256,1.0f);auto output=engine.processInputBlockForTest(input);require(output.frames()==256,"JUCE engine graph output length mismatch");
    const float expected=0.7071f*0.5f;require(std::abs(output.interleaved[0]-expected)<0.02f,"real VST3 did not process AudioEngine master graph");require(std::abs(output.interleaved[200]-expected)<0.02f,"real VST3 realtime graph failed across engine chunks");
    std::cout<<"Phase 8 real VST3 AudioEngine graph OK: "<<plugin.name<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<"Phase 8 JUCE graph test failed: "<<e.what()<<"\n";return 1;}}
