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

static AudioBuffer constantBuffer(int sampleRate,int channels,int frames,float left,float right){
    AudioBuffer out;out.sampleRate=sampleRate;out.channels=channels;
    out.interleaved.resize(static_cast<std::size_t>(frames)*static_cast<std::size_t>(channels));
    for(int frame=0;frame<frames;++frame){
        const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels);
        out.interleaved[base]=left;
        if(channels>1)out.interleaved[base+1]=right;
        for(int channel=2;channel<channels;++channel)out.interleaved[base+static_cast<std::size_t>(channel)]=left;
    }
    return out;
}

static double tailMeanAbs(const AudioBuffer& audio,int channel){
    const auto frames=static_cast<int>(audio.frames());
    const int start=frames/2;
    double sum=0.0;int count=0;
    for(int frame=start;frame<frames;++frame){
        sum+=std::abs(audio.interleaved[static_cast<std::size_t>(frame)*audio.channels+static_cast<std::size_t>(channel)]);
        ++count;
    }
    return count?sum/count:0.0;
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
        require(std::any_of(descriptors.begin(),descriptors.end(),[](const auto&d){return d.identifier=="flow.compressor"&&d.builtin&&!d.instrument;}),"FLOW Compressor descriptor missing");

        auto compressor=makeBuiltinPlugin("flow.compressor");
        require(compressor.identifier=="flow.compressor"&&compressor.name=="FLOW Compressor","FLOW Compressor identity");
        const auto params=builtinPluginParameterDescriptors("flow.compressor");
        require(params.size()==6&&compressor.parameters.size()==6,"FLOW Compressor parameter contract");

        PluginHost host;std::string error;

        auto below=constantBuffer(48000,2,24000,0.05f,0.05f);
        auto belowProcessed=below;
        setPluginParameter(compressor,"attack_ms",0.1f);
        setPluginParameter(compressor,"knee_db",0.0f);
        require(host.process(belowProcessed,compressor,error),error.c_str());
        require(std::abs(tailMeanAbs(belowProcessed,0)-0.05)<0.001,"signal below threshold should remain unchanged");

        auto strong=makeBuiltinPlugin("flow.compressor");
        setPluginParameter(strong,"threshold_db",-18.0f);
        setPluginParameter(strong,"ratio",4.0f);
        setPluginParameter(strong,"attack_ms",0.1f);
        setPluginParameter(strong,"release_ms",120.0f);
        setPluginParameter(strong,"knee_db",0.0f);
        auto loud=constantBuffer(48000,2,48000,0.5f,0.5f);
        require(host.process(loud,strong,error),error.c_str());
        const double compressed=tailMeanAbs(loud,0);
        require(compressed>0.16&&compressed<0.20,"FLOW Compressor steady-state ratio/threshold behavior is incorrect");

        auto linked=makeBuiltinPlugin("flow.compressor");
        setPluginParameter(linked,"threshold_db",-24.0f);
        setPluginParameter(linked,"ratio",8.0f);
        setPluginParameter(linked,"attack_ms",0.1f);
        setPluginParameter(linked,"knee_db",0.0f);
        auto stereo=constantBuffer(48000,2,48000,0.8f,0.08f);
        require(host.process(stereo,linked,error),error.c_str());
        const double leftGain=tailMeanAbs(stereo,0)/0.8;
        const double rightGain=tailMeanAbs(stereo,1)/0.08;
        require(std::abs(leftGain-rightGain)<0.002,"FLOW Compressor stereo link must apply one gain envelope to both channels");

        auto makeup=strong;
        setPluginParameter(makeup,"makeup_db",6.0f);
        auto withMakeup=constantBuffer(48000,2,48000,0.5f,0.5f);
        require(host.process(withMakeup,makeup,error),error.c_str());
        require(tailMeanAbs(withMakeup,0)>compressed*1.9&&tailMeanAbs(withMakeup,0)<compressed*2.1,"FLOW Compressor makeup gain regression");

        auto parity=makeBuiltinPlugin("flow.compressor");
        setPluginParameter(parity,"threshold_db",-20.0f);
        setPluginParameter(parity,"ratio",3.5f);
        setPluginParameter(parity,"attack_ms",7.0f);
        setPluginParameter(parity,"release_ms",180.0f);
        setPluginParameter(parity,"knee_db",4.0f);
        setPluginParameter(parity,"makeup_db",1.5f);
        auto source=constantBuffer(48000,2,8192,0.42f,0.21f);
        auto offline=source;
        require(host.process(offline,parity,error),error.c_str());

        auto realtime=source.interleaved;
        std::vector<RealtimePluginIssue> issues;
        RealtimePluginChain chain;
        require(chain.prepare({parity},nullptr,48000,2,128,&issues),"FLOW Compressor realtime prepare failed");
        require(issues.empty(),"FLOW Compressor realtime prepare reported issues");
        chain.process(realtime.data(),8192);
        require(maxDiff(offline.interleaved,realtime)<2.0e-5f,"FLOW Compressor offline/realtime parity regression");

        auto oneBlock=source.interleaved;
        auto chunks=source.interleaved;
        RealtimePluginChain largeChain,smallChain;
        issues.clear();require(largeChain.prepare({parity},nullptr,48000,2,8192,&issues),"large-block compressor prepare");
        issues.clear();require(smallChain.prepare({parity},nullptr,48000,2,64,&issues),"small-block compressor prepare");
        largeChain.process(oneBlock.data(),8192);
        for(int frame=0;frame<8192;frame+=64)smallChain.process(chunks.data()+static_cast<std::size_t>(frame)*2,64);
        require(maxDiff(oneBlock,chunks)<2.0e-5f,"FLOW Compressor must be block-size independent");
        for(float x:chunks)require(std::isfinite(x),"FLOW Compressor produced non-finite output");

        Project project;project.name="Phase 13 Compressor persistence";project.master.plugins.push_back(parity);
        const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase13_compressor.flow";
        ProjectSerializer::save(project,path);
        auto reopened=ProjectSerializer::load(path,false);
        require(reopened.formatVersion==11,"FLOW Compressor project persistence must remain .flow v11");
        require(reopened.master.plugins.size()==1&&reopened.master.plugins.front().identifier=="flow.compressor","FLOW Compressor identity must survive save/reopen");
        require(std::abs(pluginParameterValue(reopened.master.plugins.front(),"ratio",0.0f)-3.5f)<0.001f,"FLOW Compressor parameters must survive save/reopen");
        std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");

        std::cout<<"FLOWDAW Phase 13.2 FLOW Compressor tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 13.2 FLOW Compressor tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
