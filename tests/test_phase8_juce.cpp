#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/PluginHost.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace flowdaw;

static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static std::filesystem::path bundleFromBinary(std::filesystem::path p){for(int i=0;i<8&&!p.empty();++i){auto ext=p.extension().string();if(ext==".vst3"||ext==".component")return p;p=p.parent_path();}return{};}

int main(int argc,char**argv){try{
    require(argc>2,"effect + instrument plugin binary path arguments missing");auto bundle=bundleFromBinary(argv[1]);auto instrumentBundle=bundleFromBinary(argv[2]);require(!bundle.empty()&&!instrumentBundle.empty(),"could not locate enclosing plugin bundles");
    std::string error;auto found=scanPluginsWithJuce({bundle.parent_path(),instrumentBundle.parent_path()},error);require(error.empty(),"JUCE scan returned error");require(found.size()>=2,"JUCE did not discover both Phase 8 fixtures");
    auto effectIt=std::find_if(found.begin(),found.end(),[](auto const&d){return d.name=="FLOWDAW Test Plugin";});
    auto instrumentIt=std::find_if(found.begin(),found.end(),[](auto const&d){return d.name=="FLOWDAW Test Instrument";});
    require(effectIt!=found.end(),"JUCE effect fixture missing");require(instrumentIt!=found.end(),"JUCE instrument fixture missing");require(!effectIt->instrument,"effect fixture misclassified as instrument");require(instrumentIt->instrument,"synth fixture was not classified as instrument");
    PluginInstance plugin;plugin.format=effectIt->format;plugin.identifier=effectIt->identifier;plugin.name=effectIt->name;plugin.enabled=true;plugin.bypass=false;plugin.wet=1.0f;
    auto host=std::make_shared<PluginHost>();host->registerBackend(makeJucePluginBackend());AudioEngine engine;engine.configureExternalDevice(48000,128,true);engine.setPluginHost(host);engine.setInputMonitoring(true);
    Project project;project.master.plugins.push_back(plugin);engine.publish(project);
    AudioBuffer input;input.sampleRate=48000;input.channels=1;input.interleaved.assign(256,1.0f);auto output=engine.processInputBlockForTest(input);require(output.frames()==256,"JUCE engine graph output length mismatch");
    const float expected=0.7071f*0.5f;require(std::abs(output.interleaved[0]-expected)<0.02f,"real external plugin did not process AudioEngine master graph");require(std::abs(output.interleaved[200]-expected)<0.02f,"real external plugin realtime graph failed across engine chunks");

    engine.stop();engine.setInputMonitoring(false);
    Project routed;
    SampleAsset sample;sample.audio=std::make_shared<AudioBuffer>();sample.audio->sampleRate=48000;sample.audio->channels=1;sample.audio->interleaved.assign(512,1.0f);const Id sampleId=sample.id;routed.samples.push_back(sample);
    Bus bus;bus.name="Real VST3 Bus";bus.mixer.plugins.push_back(plugin);const Id busId=bus.id;routed.buses.push_back(bus);
    Track track;track.name="Real VST3 Track";track.outputBusId=busId;track.mixer.plugins.push_back(plugin);Clip clip;clip.sampleId=sampleId;clip.sourceLength=512;clip.lengthTicks=960;track.clips.push_back(clip);routed.tracks.push_back(track);
    engine.publish(routed);engine.play();auto routedOutput=engine.renderDeviceBlockForTest(128);
    const float routedExpected=std::sqrt(0.5f)*0.25f;
    require(std::abs(routedOutput.interleaved[0]-routedExpected)<0.02f,"real external plugin track->bus chain did not process both insert stages");
    require(std::abs(routedOutput.interleaved[100]-routedExpected)<0.02f,"real external plugin track->bus routing was not stable across the device block");

    auto bounced=renderProjectOffline(routed,0.0,host);
    require(bounced.frames()>=128,"real external plugin export ended too early");
    require(std::abs(bounced.interleaved[0]-routedExpected)<0.02f,"real external plugin was not executed during project export");
    require(std::abs(bounced.interleaved[100]-routedExpected)<0.02f,"real external plugin export did not preserve track->bus processing");

    Project instrumentProject;instrumentProject.transport.bpm=120.0;
    Pattern midiPattern;midiPattern.name="Real VST3 MIDI";MidiNote midiNote;midiNote.startTick=0;midiNote.lengthTicks=kPPQ/2;midiNote.pitch=60;midiNote.velocity=1.0f;midiPattern.midiNotes.push_back(midiNote);const Id midiPatternId=midiPattern.id;instrumentProject.patterns.push_back(midiPattern);
    Track instrumentTrack;instrumentTrack.name="Real VST3 Instrument";instrumentTrack.externalInstrumentEnabled=true;instrumentTrack.externalInstrument.format=instrumentIt->format;instrumentTrack.externalInstrument.identifier=instrumentIt->identifier;instrumentTrack.externalInstrument.name=instrumentIt->name;PatternPlacement midiPlacement;midiPlacement.patternId=midiPatternId;instrumentTrack.patternClips.push_back(midiPlacement);instrumentProject.tracks.push_back(instrumentTrack);
    engine.stop();engine.publish(instrumentProject);auto instrumentBounce=engine.renderOffline(16000);
    double noteEnergy=0.0,afterEnergy=0.0;for(SampleIndex f=1000;f<10000;++f)noteEnergy+=std::abs(instrumentBounce.interleaved[static_cast<std::size_t>(f*2)]);for(SampleIndex f=13000;f<15500;++f)afterEnergy+=std::abs(instrumentBounce.interleaved[static_cast<std::size_t>(f*2)]);
    require(noteEnergy>100.0,"real external plugin instrument did not receive note-on MIDI from Pattern");
    require(afterEnergy<0.001,"real external plugin instrument did not receive sample-accurate note-off MIDI");

    std::cout<<"Phase 8 real VST3 realtime + offline graph + instrument MIDI OK: "<<plugin.name<<" / "<<instrumentIt->name<<"\\n";return 0;
}catch(const std::exception&e){std::cerr<<"Phase 8 JUCE graph test failed: "<<e.what()<<"\n";return 1;}}
