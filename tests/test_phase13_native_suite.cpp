#include "flowdaw/PluginHost.hpp"
#include "flowdaw/RealtimePluginGraph.hpp"
#include "flowdaw/Serialization.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace flowdaw;

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}

static AudioBuffer impulse(int sr,int channels,int frames,float amp=1.0f){
    AudioBuffer b;b.sampleRate=sr;b.channels=channels;b.interleaved.assign(static_cast<std::size_t>(frames)*channels,0.0f);
    for(int c=0;c<channels;++c)b.interleaved[static_cast<std::size_t>(c)]=amp;
    return b;
}
static AudioBuffer constant(int sr,int channels,int frames,float l,float r){
    AudioBuffer b;b.sampleRate=sr;b.channels=channels;b.interleaved.resize(static_cast<std::size_t>(frames)*channels);
    for(int f=0;f<frames;++f){auto base=static_cast<std::size_t>(f)*channels;b.interleaved[base]=l;if(channels>1)b.interleaved[base+1]=r;for(int c=2;c<channels;++c)b.interleaved[base+c]=l;}
    return b;
}
static float peak(const AudioBuffer&b){float p=0.0f;for(float x:b.interleaved)p=std::max(p,std::abs(x));return p;}
static double energy(const AudioBuffer&b,int start=0){
    double e=0.0;const int frames=static_cast<int>(b.frames());
    for(int f=std::max(0,start);f<frames;++f)for(int c=0;c<b.channels;++c){float x=b.interleaved[static_cast<std::size_t>(f)*b.channels+c];e+=static_cast<double>(x)*x;}
    return e;
}
static void finite(const AudioBuffer&b,const char*message){for(float x:b.interleaved)require(std::isfinite(x),message);}

int main(){
    try{
        PluginHost host;std::string error;

        // 13.4 Saturator: nonlinear transfer, modes/tone metadata, finite output.
        auto sat=makeBuiltinPlugin("flow.saturator");
        require(sat.identifier=="flow.saturator","FLOW Saturator identity");
        require(builtinPluginParameterDescriptors("flow.saturator").size()==3,"FLOW Saturator parameter contract");
        auto satAudio=constant(48000,2,2048,0.35f,-0.35f);
        auto satDry=satAudio;
        setPluginParameter(sat,"drive_db",18.0f);
        setPluginParameter(sat,"mode",0.0f);
        require(host.process(satAudio,sat,error),error.c_str());
        finite(satAudio,"FLOW Saturator non-finite");
        require(std::abs(satAudio.interleaved[0]-satDry.interleaved[0])>0.05f,"FLOW Saturator must alter driven signal");

        // 13.5 Reverb: impulse creates delayed tail and default wet mix is sensible.
        auto reverb=makeBuiltinPlugin("flow.reverb");
        require(std::abs(reverb.wet-0.25f)<0.001f,"FLOW Reverb default wet");
        auto revAudio=impulse(48000,2,48000,0.6f);
        require(host.process(revAudio,reverb,error),error.c_str());
        finite(revAudio,"FLOW Reverb non-finite");
        require(energy(revAudio,2000)>0.00001,"FLOW Reverb must create a tail");

        // 13.6 Delay: free-time echo and BPM-sync calculation both produce delayed energy.
        auto delay=makeBuiltinPlugin("flow.delay");
        setPluginParameter(delay,"time_ms",50.0f);
        setPluginParameter(delay,"feedback",0.45f);
        auto delayAudio=impulse(48000,2,12000,0.8f);
        require(host.process(delayAudio,delay,error),error.c_str());
        const int expected=2400;
        require(std::abs(delayAudio.interleaved[static_cast<std::size_t>(expected)*2])>0.05f,"FLOW Delay free-time echo missing");
        auto syncDelay=makeBuiltinPlugin("flow.delay");
        setPluginParameter(syncDelay,"sync_bpm",120.0f);setPluginParameter(syncDelay,"sync_beats",0.5f);setPluginParameter(syncDelay,"feedback",0.0f);
        auto syncAudio=impulse(48000,2,16000,0.8f);
        require(host.process(syncAudio,syncDelay,error),error.c_str());
        require(std::abs(syncAudio.interleaved[static_cast<std::size_t>(12000)*2])>0.05f,"FLOW Delay BPM-sync echo missing");

        // 13.7 Chorus: modulated delay creates non-zero wet signal after its base delay.
        auto chorus=makeBuiltinPlugin("flow.chorus");
        auto chorusAudio=constant(48000,2,8192,0.2f,-0.2f);
        require(host.process(chorusAudio,chorus,error),error.c_str());
        finite(chorusAudio,"FLOW Chorus non-finite");
        require(energy(chorusAudio,1500)>0.01,"FLOW Chorus wet signal missing");

        // 13.8 Gate: low signal is strongly attenuated while high signal opens.
        auto gate=makeBuiltinPlugin("flow.gate");
        setPluginParameter(gate,"threshold_db",-30.0f);setPluginParameter(gate,"range_db",-60.0f);setPluginParameter(gate,"attack_ms",0.1f);setPluginParameter(gate,"hold_ms",0.0f);setPluginParameter(gate,"release_ms",20.0f);
        auto low=constant(48000,2,12000,0.001f,0.001f);
        require(host.process(low,gate,error),error.c_str());
        require(peak(low)<0.0001f,"FLOW Gate must attenuate signal below threshold");
        auto high=constant(48000,2,12000,0.3f,0.3f);
        require(host.process(high,gate,error),error.c_str());
        require(peak(high)>0.25f,"FLOW Gate must open above threshold");

        // 13.9 Utility: mono, polarity, swap/balance/width and gain use one first-party insert.
        auto utility=makeBuiltinPlugin("flow.utility");
        setPluginParameter(utility,"mono",1.0f);setPluginParameter(utility,"gain_db",6.0f);
        auto utilAudio=constant(48000,2,64,0.4f,0.0f);
        require(host.process(utilAudio,utility,error),error.c_str());
        require(std::abs(utilAudio.interleaved[0]-utilAudio.interleaved[1])<0.0001f,"FLOW Utility mono must collapse stereo");
        require(utilAudio.interleaved[0]>0.38f,"FLOW Utility gain not applied");

        auto polarity=makeBuiltinPlugin("flow.utility");setPluginParameter(polarity,"polarity",1.0f);
        auto polarityAudio=constant(48000,2,8,0.25f,-0.1f);require(host.process(polarityAudio,polarity,error),error.c_str());
        require(polarityAudio.interleaved[0]<0.0f&&polarityAudio.interleaved[1]>0.0f,"FLOW Utility polarity inversion failed");

        // Prepared realtime smoke for every new processor. No external host is required.
        for(const auto&id:{"flow.saturator","flow.reverb","flow.delay","flow.chorus","flow.gate","flow.utility"}){
            auto p=makeBuiltinPlugin(id);RealtimePluginChain chain;std::vector<RealtimePluginIssue>issues;
            require(chain.prepare({p},nullptr,48000,2,128,&issues),"native suite realtime prepare failed");
            require(issues.empty(),"native suite realtime issue");
            float block[256]{};block[0]=0.5f;block[1]=-0.25f;chain.process(block,128);
            for(float x:block)require(std::isfinite(x),"native suite realtime non-finite");
        }

        // Project compatibility: all new effects persist in the existing generic PluginInstance schema.
        Project project;project.name="Phase 13 native suite persistence";
        for(const auto&id:{"flow.saturator","flow.reverb","flow.delay","flow.chorus","flow.gate","flow.utility"})project.master.plugins.push_back(makeBuiltinPlugin(id));
        const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase13_native_suite.flow";
        ProjectSerializer::save(project,path);auto reopened=ProjectSerializer::load(path,false);
        require(reopened.formatVersion==11,"Phase 13 native suite must remain .flow v11");
        require(reopened.master.plugins.size()==6,"Phase 13 native suite persistence count");
        require(reopened.master.plugins[1].identifier=="flow.reverb"&&std::abs(reopened.master.plugins[1].wet-0.25f)<0.001f,"native suite identity/wet persistence");
        std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");

        std::cout<<"FLOWDAW Phase 13.4-13.9 native suite tests: PASS\n";
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 13.4-13.9 native suite tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
