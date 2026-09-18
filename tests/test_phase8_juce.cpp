#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Export.hpp"
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

    engine.stop();engine.setInputMonitoring(false);
    Project routed;
    SampleAsset sample;sample.audio=std::make_shared<AudioBuffer>();sample.audio->sampleRate=48000;sample.audio->channels=1;sample.audio->interleaved.assign(512,1.0f);const Id sampleId=sample.id;routed.samples.push_back(sample);
    Bus bus;bus.name="Real VST3 Bus";bus.mixer.plugins.push_back(plugin);const Id busId=bus.id;routed.buses.push_back(bus);
    Track track;track.name="Real VST3 Track";track.outputBusId=busId;track.mixer.plugins.push_back(plugin);Clip clip;clip.sampleId=sampleId;clip.sourceLength=512;clip.lengthTicks=960;track.clips.push_back(clip);routed.tracks.push_back(track);
    engine.publish(routed);engine.play();auto routedOutput=engine.renderDeviceBlockForTest(128);
    const float routedExpected=std::sqrt(0.5f)*0.25f;
    require(std::abs(routedOutput.interleaved[0]-routedExpected)<0.02f,"real VST3 track->bus chain did not process both insert stages");
    require(std::abs(routedOutput.interleaved[100]-routedExpected)<0.02f,"real VST3 track->bus routing was not stable across the device block");

    auto bounced=renderProjectOffline(routed,0.0,host);
    require(bounced.frames()>=128,"real VST3 export ended too early");
    require(std::abs(bounced.interleaved[0]-routedExpected)<0.02f,"real VST3 was not executed during project export");
    require(std::abs(bounced.interleaved[100]-routedExpected)<0.02f,"real VST3 export did not preserve track->bus processing");

    std::cout<<"Phase 8 real VST3 realtime + offline graph OK: "<<plugin.name<<"\\n";return 0;
}catch(const std::exception&e){std::cerr<<"Phase 8 JUCE graph test failed: "<<e.what()<<"\n";return 1;}}
