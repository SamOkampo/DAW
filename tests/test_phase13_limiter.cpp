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

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

static AudioBuffer constantBuffer(int sampleRate,int channels,int frames,float value){
    AudioBuffer out;out.sampleRate=sampleRate;out.channels=channels;
    out.interleaved.assign(static_cast<std::size_t>(frames)*static_cast<std::size_t>(channels),value);
    return out;
}

static float maxAbs(const std::vector<float>& data){
    float peak=0.0f;
    for(float x:data)peak=std::max(peak,std::abs(x));
    return peak;
}

static float maxDiff(const std::vector<float>&a,const std::vector<float>&b){
    require(a.size()==b.size(),"buffer sizes differ");
    float d=0.0f;
    for(std::size_t i=0;i<a.size();++i)d=std::max(d,std::abs(a[i]-b[i]));
    return d;
}

int main(){
    try{
        const auto descriptors=builtinPluginDescriptors();
        require(std::any_of(descriptors.begin(),descriptors.end(),[](const auto&d){return d.identifier=="flow.limiter"&&d.builtin&&!d.instrument;}),"FLOW Limiter descriptor missing");

        auto limiter=makeBuiltinPlugin("flow.limiter");
        require(limiter.identifier=="flow.limiter"&&limiter.name=="FLOW Limiter","FLOW Limiter identity");
        const auto params=builtinPluginParameterDescriptors("flow.limiter");
        require(params.size()==4&&limiter.parameters.size()==4,"FLOW Limiter parameter contract");

        PluginHost host;std::string error;
        auto processor=host.createProcessor(limiter,error);
        require(processor!=nullptr,error.c_str());
        require(processor->prepare(48000,2,error),error.c_str());
        require(processor->supportsRealtimeProcessing(),"FLOW Limiter must be realtime-capable");
        require(processor->latencySamples()==144,"3 ms FLOW Limiter lookahead must report 144 samples at 48 kHz");

        auto ceilingLimiter=makeBuiltinPlugin("flow.limiter");
        setPluginParameter(ceilingLimiter,"ceiling_db",-6.0f);
        setPluginParameter(ceilingLimiter,"lookahead_ms",3.0f);
        setPluginParameter(ceilingLimiter,"release_ms",80.0f);
        auto loud=constantBuffer(48000,2,4096,0.9f);
        require(host.process(loud,ceilingLimiter,error),error.c_str());
        const float ceiling=std::pow(10.0f,-6.0f/20.0f);
        require(maxAbs(loud.interleaved)<=ceiling+1.0e-5f,"FLOW Limiter exceeded sample-peak ceiling");
        for(float x:loud.interleaved)require(std::isfinite(x),"FLOW Limiter produced non-finite output");

        auto driven=makeBuiltinPlugin("flow.limiter");
        setPluginParameter(driven,"ceiling_db",-6.0f);
        setPluginParameter(driven,"input_gain_db",6.0f);
        setPluginParameter(driven,"lookahead_ms",0.0f);
        auto drivenAudio=constantBuffer(48000,2,4096,0.3f);
        require(host.process(drivenAudio,driven,error),error.c_str());
        require(maxAbs(drivenAudio.interleaved)<=ceiling+1.0e-5f,"FLOW Limiter input gain path exceeded ceiling");
        require(maxAbs(drivenAudio.interleaved)>0.45f,"FLOW Limiter input gain should drive signal into limiting");

        auto parity=makeBuiltinPlugin("flow.limiter");
        setPluginParameter(parity,"ceiling_db",-2.0f);
        setPluginParameter(parity,"input_gain_db",4.0f);
        setPluginParameter(parity,"lookahead_ms",2.5f);
        setPluginParameter(parity,"release_ms",90.0f);
        AudioBuffer source;source.sampleRate=48000;source.channels=2;source.interleaved.resize(8192*2);
        for(int frame=0;frame<8192;++frame){
            const float x=(frame%257==0)?0.95f:0.18f*static_cast<float>(std::sin(frame*0.071));
            source.interleaved[static_cast<std::size_t>(frame)*2]=x;
            source.interleaved[static_cast<std::size_t>(frame)*2+1]=x*0.6f;
        }
        auto offline=source;
        require(host.process(offline,parity,error),error.c_str());

        auto realtime=source.interleaved;
        std::vector<RealtimePluginIssue> issues;
        RealtimePluginChain chain;
        require(chain.prepare({parity},nullptr,48000,2,128,&issues),"FLOW Limiter realtime prepare failed");
        require(issues.empty(),"FLOW Limiter realtime prepare reported issues");
        require(chain.latencySamples()==120,"2.5 ms FLOW Limiter latency must report 120 samples at 48 kHz");
        chain.process(realtime.data(),8192);
        require(maxDiff(offline.interleaved,realtime)<2.0e-5f,"FLOW Limiter offline/realtime parity regression");

        auto oneBlock=source.interleaved;
        auto chunks=source.interleaved;
        RealtimePluginChain largeChain,smallChain;
        issues.clear();require(largeChain.prepare({parity},nullptr,48000,2,8192,&issues),"large-block limiter prepare");
        issues.clear();require(smallChain.prepare({parity},nullptr,48000,2,64,&issues),"small-block limiter prepare");
        largeChain.process(oneBlock.data(),8192);
        for(int frame=0;frame<8192;frame+=64)smallChain.process(chunks.data()+static_cast<std::size_t>(frame)*2,64);
        require(maxDiff(oneBlock,chunks)<2.0e-5f,"FLOW Limiter must be block-size independent");

        Project project;project.name="Phase 13 Limiter persistence";project.master.plugins.push_back(parity);
        const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase13_limiter.flow";
        ProjectSerializer::save(project,path);
        auto reopened=ProjectSerializer::load(path,false);
        require(reopened.formatVersion==11,"FLOW Limiter persistence must remain .flow v11");
        require(reopened.master.plugins.size()==1&&reopened.master.plugins.front().identifier=="flow.limiter","FLOW Limiter identity must survive save/reopen");
        require(std::abs(pluginParameterValue(reopened.master.plugins.front(),"lookahead_ms",0.0f)-2.5f)<0.001f,"FLOW Limiter parameters must survive save/reopen");
        std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");

        std::cout<<"FLOWDAW Phase 13.3 FLOW Limiter tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 13.3 FLOW Limiter tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
