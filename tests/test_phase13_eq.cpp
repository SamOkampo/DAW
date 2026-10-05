#include "flowdaw/PluginHost.hpp"
#include "flowdaw/RealtimePluginGraph.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

static AudioBuffer sine(int sampleRate,int channels,int frames,float frequency,float amplitude=0.2f){
    AudioBuffer out;out.sampleRate=sampleRate;out.channels=channels;
    out.interleaved.resize(static_cast<std::size_t>(frames)*static_cast<std::size_t>(channels));
    constexpr double pi=3.14159265358979323846;
    for(int frame=0;frame<frames;++frame){
        const float x=amplitude*static_cast<float>(std::sin(2.0*pi*frequency*frame/sampleRate));
        for(int channel=0;channel<channels;++channel)
            out.interleaved[static_cast<std::size_t>(frame)*channels+static_cast<std::size_t>(channel)]=x;
    }
    return out;
}

static double rms(const AudioBuffer& audio){
    double sum=0.0;
    for(float x:audio.interleaved)sum+=static_cast<double>(x)*x;
    return audio.interleaved.empty()?0.0:std::sqrt(sum/audio.interleaved.size());
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
        require(std::any_of(descriptors.begin(),descriptors.end(),[](const auto&d){return d.identifier=="flow.eq"&&d.builtin&&!d.instrument;}),"FLOW EQ descriptor missing");

        auto eq=makeBuiltinPlugin("flow.eq");
        require(eq.identifier=="flow.eq"&&eq.name=="FLOW EQ","FLOW EQ identity");
        const auto params=builtinPluginParameterDescriptors("flow.eq");
        require(params.size()==19,"FLOW EQ must expose six frequency/gain/Q bands plus output gain");
        require(eq.parameters.size()==params.size(),"FLOW EQ defaults must be persisted");

        PluginHost host;
        std::string error;

        auto transparentInput=sine(48000,2,4096,997.0f);
        auto transparent=transparentInput;
        require(host.process(transparent,eq,error),error.c_str());
        require(maxDiff(transparentInput.interleaved,transparent.interleaved)<1.0e-5f,"zero-gain FLOW EQ should be transparent");

        auto boostedEq=makeBuiltinPlugin("flow.eq");
        setPluginParameter(boostedEq,"band4_freq",1000.0f);
        setPluginParameter(boostedEq,"band4_gain_db",12.0f);
        setPluginParameter(boostedEq,"band4_q",1.0f);
        auto dry=sine(48000,2,16384,1000.0f);
        auto boosted=dry;
        require(host.process(boosted,boostedEq,error),error.c_str());
        require(rms(boosted)>rms(dry)*1.65,"FLOW EQ 1 kHz boost is too weak");
        for(float x:boosted.interleaved)require(std::isfinite(x),"FLOW EQ produced non-finite output");

        auto offlineInput=sine(48000,2,8192,1500.0f,0.1f);
        auto offline=offlineInput;
        auto parityEq=makeBuiltinPlugin("flow.eq");
        setPluginParameter(parityEq,"band2_freq",220.0f);
        setPluginParameter(parityEq,"band2_gain_db",-5.0f);
        setPluginParameter(parityEq,"band4_freq",1500.0f);
        setPluginParameter(parityEq,"band4_gain_db",7.5f);
        setPluginParameter(parityEq,"band4_q",1.4f);
        setPluginParameter(parityEq,"output_gain_db",-1.5f);
        require(host.process(offline,parityEq,error),error.c_str());

        auto realtime=offlineInput.interleaved;
        RealtimePluginChain chain;
        std::vector<RealtimePluginIssue> issues;
        require(chain.prepare({parityEq},nullptr,48000,2,128,&issues),"FLOW EQ realtime chain prepare failed");
        require(issues.empty(),"FLOW EQ realtime prepare reported issues");
        chain.process(realtime.data(),8192);
        require(maxDiff(offline.interleaved,realtime)<2.0e-5f,"FLOW EQ offline/realtime parity regression");

        auto oneBlock=offlineInput.interleaved;
        auto chunks=offlineInput.interleaved;
        RealtimePluginChain largeChain,smallChain;
        require(largeChain.prepare({parityEq},nullptr,48000,2,8192,&issues),"large-block EQ prepare");
        issues.clear();
        require(smallChain.prepare({parityEq},nullptr,48000,2,64,&issues),"small-block EQ prepare");
        largeChain.process(oneBlock.data(),8192);
        for(int frame=0;frame<8192;frame+=64)
            smallChain.process(chunks.data()+static_cast<std::size_t>(frame)*2,64);
        require(maxDiff(oneBlock,chunks)<2.0e-5f,"FLOW EQ must be block-size independent");

        auto oldGain=makeBuiltinPlugin("flow.gain");
        setPluginParameter(oldGain,"gain",0.5f);
        float samples[]={1.0f,1.0f,-1.0f,-1.0f};
        RealtimePluginChain legacy;
        issues.clear();
        require(legacy.prepare({oldGain},nullptr,48000,2,8,&issues),"existing FLOW Gain must use prepared native processor path");
        legacy.process(samples,2);
        require(std::abs(samples[0]-0.5f)<1.0e-6f&&std::abs(samples[2]+0.5f)<1.0e-6f,"existing FLOW Gain realtime behavior changed");

        std::cout<<"FLOWDAW Phase 13.1 FLOW EQ tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 13.1 FLOW EQ tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
