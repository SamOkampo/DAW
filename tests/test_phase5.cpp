#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/PluginHost.hpp"
#include "flowdaw/ProductionAssistant.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Workflow.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static double energy(const AudioBuffer&a){double e=0;for(float x:a.interleaved)e+=std::abs(x);return e;}
static std::shared_ptr<AudioBuffer> constantAudio(float value=0.1f,SampleIndex frames=2000){auto a=std::make_shared<AudioBuffer>();a->sampleRate=48000;a->channels=1;a->interleaved.assign(static_cast<std::size_t>(frames),value);return a;}

class FakeProcessor final:public IPluginProcessor{
public:
 bool prepare(int,int,std::string&)override{return true;}
 void setState(const std::string&s)override{state_=s;}
 std::string state()const override{return state_+"!";}
 void process(AudioBuffer&b)override{for(auto&x:b.interleaved)x*=0.5f;}
private:std::string state_;
};
class FakeBackend final:public IExternalPluginBackend{
public:
 bool supports(const std::string&format)const override{return format=="vst3";}
 std::unique_ptr<IPluginProcessor> create(const PluginInstance&,std::string&)override{return std::make_unique<FakeProcessor>();}
};

int main(){
 try{
    AudioBuffer a;a.sampleRate=48000;a.channels=2;a.interleaved={.2f,-.2f,.1f,-.1f};PluginHost host;auto gain=makeBuiltinPlugin("flow.gain");setPluginParameter(gain,"gain",2.0f);std::string err;require(host.process(a,gain,err),"builtin gain processes");require(std::abs(a.interleaved[0]-.4f)<.001f,"builtin gain changes audio");
    auto width=makeBuiltinPlugin("flow.width");setPluginParameter(width,"width",0.0f);require(host.process(a,width,err),"width processes stereo");require(std::abs(a.interleaved[0]-a.interleaved[1])<.001f,"width zero collapses to mono");

    PluginInstance ext;ext.format="vst3";ext.identifier="fake.vst3";ext.name="Fake";AudioBuffer e=e; // intentionally replaced below
    AudioBuffer extAudio;extAudio.sampleRate=48000;extAudio.channels=1;extAudio.interleaved.assign(32,.2f);require(!host.process(extAudio,ext,err)&&err.find("backend")!=std::string::npos,"external plugin requires registered backend");host.registerBackend(std::make_shared<FakeBackend>());ext.opaqueState="abc";require(host.process(extAudio,ext,err),"registered backend executes external plugin");require(std::abs(extAudio.interleaved[0]-.1f)<.001f&&ext.opaqueState=="abc!","backend audio and state roundtrip");

    const auto scanRoot=std::filesystem::temp_directory_path()/"flowdaw_phase5_scan";std::filesystem::create_directories(scanRoot/"Boom.vst3");{std::ofstream f(scanRoot/"Space.component");f<<"descriptor only";}auto found=scanPluginPaths({scanRoot});require(found.size()==2,"scanner finds VST3 and AU bundles without loading them");require((found[0].format=="au"||found[1].format=="au")&&(found[0].format=="vst3"||found[1].format=="vst3"),"scanner identifies both formats");std::filesystem::remove_all(scanRoot);

    Project p;p.name="Phase 5";p.master.volume=1.0f;p.master.plugins.push_back(makeBuiltinPlugin("flow.softclip"));setPluginParameter(p.master.plugins[0],"drive",.2f);p.master.plugins[0].opaqueState="master-state";Track t;t.name="Beat";t.mixer.volume=1.2f;t.mixer.plugins.push_back(makeBuiltinPlugin("flow.gain"));setPluginParameter(t.mixer.plugins[0],"gain",.8f);const Id tid=t.id;p.tracks.push_back(t);Bus b;b.name="FX";b.mixer.plugins.push_back(ext);p.buses.push_back(b);Pattern pat;pat.name="Drums";pat.swing=0;pat.humanize=0;DrumLane lane;lane.name="kick";pat.lanes.push_back(lane);p.patterns.push_back(pat);
    const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase5_v10.flow";ProjectSerializer::save(p,path);auto loaded=ProjectSerializer::load(path,false);require(loaded.formatVersion==10,"project saves/loads as v10");require(loaded.master.plugins.size()==1&&loaded.tracks[0].mixer.plugins.size()==1&&loaded.buses[0].mixer.plugins.size()==1,"plugin racks persist");require(loaded.master.plugins[0].opaqueState=="master-state"&&std::abs(pluginParameterValue(loaded.tracks[0].mixer.plugins[0],"gain",0)-.8f)<.001f,"plugin state and parameters persist");ProjectSerializer::save(loaded,path);require(std::filesystem::exists(path.string()+".bak"),"safe save creates a backup of the previous project");std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");

    const auto legacyPath=std::filesystem::temp_directory_path()/"flowdaw_phase5_legacy_v9.flow";{std::ofstream f(legacyPath);f<<"FLOWDAW_PROJECT 9\nNAME \"legacy\"\nSAMPLE_RATE 48000\nBPM 90\nPLAYHEAD 0\nMASTER 1 0\nSAMPLES 0\nTRACKS 0\nPATTERNS 0\nBUSES 0\nAUTOMATION 0\nEND\n";}auto legacy=ProjectSerializer::load(legacyPath,false);std::filesystem::remove(legacyPath);require(legacy.formatVersion==10&&legacy.master.plugins.empty(),"v9 project migrates safely to v10");

    auto context=buildProjectContext(p);require(context.find("tracks=1")!=std::string::npos&&context.find("Beat")!=std::string::npos,"assistant context describes current project");auto suggestions=analyzeProductionContext(p);require(!suggestions.empty(),"assistant returns contextual suggestions");bool foundTrim=false;for(auto const&s:suggestions)if(s.action==AssistantAction::TrimTrack){Project copy=p;std::string result;require(applyAssistantSuggestion(copy,s,result),"assistant suggestion applies explicitly");require(copy.findTrack(tid)&&copy.findTrack(tid)->mixer.volume<=1.0f,"assistant trim changes intended track");foundTrim=true;break;}require(foundTrim,"assistant detects hot track");

    Project broken=p;broken.tracks[0].outputBusId=999999;auto issues=validateProject(broken);bool brokenRoute=false;for(auto const&i:issues)if(i.code=="broken-route")brokenRoute=true;require(brokenRoute,"project health catches broken routing");auto commands=workflowCommands("vocal");require(commands.size()==1&&commands[0].id=="arm-vocal","command search filters workflow actions");std::string workflowResult;require(executeWorkflowCommand(broken,"arm-vocal",0,workflowResult),"workflow command executes");bool armed=false;for(auto const&tr:broken.tracks)if(tr.name=="VOCAL"&&tr.armed)armed=true;require(armed,"arm vocal command creates/arms vocal track");

    Project audioProject;SampleAsset s;s.audio=constantAudio(.1f,4000);const Id sid=s.id;audioProject.samples.push_back(s);Track at;Clip c;c.sampleId=sid;c.sourceLength=4000;c.lengthTicks=960;at.clips.push_back(c);audioProject.tracks.push_back(at);AudioEngine dryEngine;dryEngine.publish(audioProject);auto dryAudio=dryEngine.renderOffline(3000);audioProject.master.plugins.push_back(makeBuiltinPlugin("flow.gain"));setPluginParameter(audioProject.master.plugins.back(),"gain",.5f);AudioEngine wetEngine;wetEngine.publish(audioProject);auto wetAudio=wetEngine.renderOffline(3000);require(energy(wetAudio)<energy(dryAudio)*.6,"native master plugin rack affects realtime/offline engine graph");

    std::cout<<"FLOWDAW Phase 5 assist/plugin/workflow tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
