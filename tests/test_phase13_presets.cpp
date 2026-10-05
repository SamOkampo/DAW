#include "flowdaw/NativePresets.hpp"
#include "flowdaw/PluginHost.hpp"
#include "flowdaw/Serialization.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace flowdaw;

static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}

int main(){
    try{
        const auto presets=builtinNativeEffectPresets();
        require(presets.size()>=15,"native effect preset catalog too small");
        std::set<std::string> covered;
        for(const auto&p:presets){
            require(p.schemaVersion==1,"native effect preset schema");
            require(!p.id.empty()&&!p.name.empty()&&!p.category.empty(),"native effect preset metadata");
            require(isSupportedNativeEffectIdentifier(p.pluginIdentifier),"native effect preset plugin unsupported");
            auto plugin=instantiateNativeEffectPreset(p);
            require(plugin.format=="builtin"&&plugin.identifier==p.pluginIdentifier,"preset instantiate identity");
            require(std::abs(plugin.wet-p.wet)<0.0001f,"preset wet did not instantiate");
            covered.insert(p.pluginIdentifier);
        }
        for(const auto&id:{"flow.eq","flow.compressor","flow.limiter","flow.saturator","flow.reverb","flow.delay","flow.chorus","flow.gate","flow.utility"})
            require(covered.count(id)==1,"Phase 13 plugin lacks first-party preset coverage");

        const auto it=std::find_if(presets.begin(),presets.end(),[](const auto&p){return p.id=="reverb-wide-hall";});
        require(it!=presets.end(),"expected reverb preset missing");
        const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase13_effect.flowfxpreset";
        saveNativeEffectPreset(*it,path);
        const auto loaded=loadNativeEffectPreset(path);
        require(loaded.id==it->id&&loaded.pluginIdentifier==it->pluginIdentifier,"effect preset file roundtrip identity");
        require(loaded.parameters.size()==it->parameters.size(),"effect preset file roundtrip parameter count");
        require(std::abs(loaded.wet-it->wet)<0.0001f,"effect preset file roundtrip wet");
        std::filesystem::remove(path);

        auto plugin=instantiateNativeEffectPreset(*it);
        Project project;project.name="Phase 13 preset project";project.master.plugins.push_back(plugin);
        const auto projectPath=std::filesystem::temp_directory_path()/"flowdaw_phase13_preset_project.flow";
        ProjectSerializer::save(project,projectPath);
        auto reopened=ProjectSerializer::load(projectPath,false);
        require(reopened.formatVersion==11,"effect preset project must remain .flow v11");
        require(reopened.master.plugins.size()==1&&reopened.master.plugins.front().identifier=="flow.reverb","effect preset project identity");
        require(std::abs(reopened.master.plugins.front().wet-it->wet)<0.0001f,"effect preset project wet persistence");
        std::filesystem::remove(projectPath);std::filesystem::remove(projectPath.string()+".bak");

        PluginHost host;std::string error;AudioBuffer audio;audio.sampleRate=48000;audio.channels=2;audio.interleaved.assign(48000*2,0.0f);audio.interleaved[0]=0.5f;audio.interleaved[1]=0.5f;
        require(host.process(audio,plugin,error),error.c_str());
        for(float x:audio.interleaved)require(std::isfinite(x),"effect preset produced non-finite audio");

        std::cout<<"FLOWDAW Phase 13.10 native effect preset tests: PASS\n";return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 13.10 native effect preset tests: FAIL: "<<e.what()<<"\n";return 1;
    }
}
