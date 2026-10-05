#include "flowdaw/PluginHost.hpp"
#include "flowdaw/RealtimePluginGraph.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace flowdaw;

static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
static bool finiteBlock(const std::vector<float>&v){for(float x:v)if(!std::isfinite(x))return false;return true;}

int main(){
    try{
        const auto descriptors=builtinPluginDescriptors();
        require(descriptors.size()>=12,"expected Phase 13 native descriptor set");

        const std::vector<int> rates{44100,48000,96000};
        const std::vector<SampleIndex> blocks{1,17,64,257,1024};

        for(const auto&descriptor:descriptors){
            if(!descriptor.builtin||descriptor.instrument)continue;
            auto plugin=makeBuiltinPlugin(descriptor.identifier);
            for(int rate:rates){
                for(auto block:blocks){
                    RealtimePluginChain chain;std::vector<RealtimePluginIssue>issues;
                    require(chain.prepare({plugin},nullptr,rate,2,std::max<SampleIndex>(1,block),&issues),"native audit prepare failed");
                    require(issues.empty(),"native audit prepare issue");
                    std::vector<float> audio(static_cast<std::size_t>(block)*2);
                    for(SampleIndex f=0;f<block;++f){
                        const float x=0.45f*std::sin(static_cast<float>(f)*0.071f)+((f%29)==0?0.35f:0.0f);
                        audio[static_cast<std::size_t>(f)*2]=x;
                        audio[static_cast<std::size_t>(f)*2+1]=x*0.73f;
                    }
                    chain.process(audio.data(),block);
                    require(finiteBlock(audio),"native audit produced NaN/Inf");
                    chain.reset();
                    std::fill(audio.begin(),audio.end(),0.0f);
                    chain.process(audio.data(),block);
                    require(finiteBlock(audio),"native audit zero/reset produced NaN/Inf");
                    for(float x:audio)require(std::abs(x)<2.0f,"native audit unstable zero/reset response");
                }
            }
        }

        // Bypass must leave the signal untouched.
        auto bypass=makeBuiltinPlugin("flow.saturator");bypass.bypass=true;
        RealtimePluginChain bypassChain;std::vector<RealtimePluginIssue>issues;
        require(bypassChain.prepare({bypass},nullptr,48000,2,64,&issues),"bypass chain prepare");
        float bypassAudio[8]{0.2f,-0.2f,0.4f,-0.4f,0.1f,-0.1f,0.3f,-0.3f};
        const float bypassCopy[8]{0.2f,-0.2f,0.4f,-0.4f,0.1f,-0.1f,0.3f,-0.3f};
        bypassChain.process(bypassAudio,4);
        for(int i=0;i<8;++i)require(std::abs(bypassAudio[i]-bypassCopy[i])<1.0e-7f,"native bypass changed audio");

        // Generic wet=0 returns dry audio for zero-latency native processors.
        auto dryEq=makeBuiltinPlugin("flow.eq");dryEq.wet=0.0f;
        RealtimePluginChain wetChain;issues.clear();
        require(wetChain.prepare({dryEq},nullptr,48000,2,64,&issues),"wet-zero chain prepare");
        float wetAudio[8]{0.2f,-0.1f,0.4f,-0.2f,-0.3f,0.15f,0.1f,-0.05f};
        const float wetCopy[8]{0.2f,-0.1f,0.4f,-0.2f,-0.3f,0.15f,0.1f,-0.05f};
        wetChain.process(wetAudio,4);
        for(int i=0;i<8;++i)require(std::abs(wetAudio[i]-wetCopy[i])<1.0e-6f,"generic wet=0 failed for native processor");

        // Limiter latency is explicit and therefore participates in PDC.
        auto limiter=makeBuiltinPlugin("flow.limiter");setPluginParameter(limiter,"lookahead_ms",3.0f);
        RealtimePluginChain limiterChain;issues.clear();
        require(limiterChain.prepare({limiter},nullptr,48000,2,256,&issues),"limiter audit prepare");
        require(limiterChain.latencySamples()==144,"FLOW Limiter latency reporting changed");

        // Extreme but finite parameter-bound signals must stay finite.
        for(const auto&id:{"flow.eq","flow.compressor","flow.limiter","flow.saturator","flow.reverb","flow.delay","flow.chorus","flow.gate","flow.utility"}){
            auto plugin=makeBuiltinPlugin(id);
            for(auto&parameter:plugin.parameters){
                const auto meta=builtinPluginParameterDescriptors(plugin.identifier);
                auto it=std::find_if(meta.begin(),meta.end(),[&](const auto&d){return d.id==parameter.id;});
                if(it!=meta.end())parameter.value=it->maxValue;
            }
            RealtimePluginChain chain;issues.clear();
            require(chain.prepare({plugin},nullptr,96000,2,511,&issues),"max-parameter native prepare failed");
            std::vector<float> audio(511*2,0.95f);
            chain.process(audio.data(),511);
            require(finiteBlock(audio),"max-parameter native processing generated NaN/Inf");
        }

        std::cout<<"FLOWDAW Phase 13.12 DSP/realtime audit tests: PASS\n";
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 13.12 DSP/realtime audit tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
