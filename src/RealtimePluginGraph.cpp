#include "flowdaw/RealtimePluginGraph.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace flowdaw {

void RealtimeDelayLine::prepare(int delaySamples,int channels){
    delaySamples_=std::max(0,delaySamples);channels_=std::max(0,channels);writeFrame_=0;
    const auto samples=static_cast<std::size_t>(delaySamples_)*static_cast<std::size_t>(channels_);
    storage_.assign(samples,0.0f);
}

void RealtimeDelayLine::reset() noexcept{
    std::fill(storage_.begin(),storage_.end(),0.0f);writeFrame_=0;
}

void RealtimeDelayLine::addDelayed(const float*source,float*destination,SampleIndex frames,float gain) noexcept{
    if(!source||!destination||frames<=0||channels_<=0)return;
    if(delaySamples_<=0){
        const auto count=static_cast<std::size_t>(frames)*static_cast<std::size_t>(channels_);
        for(std::size_t i=0;i<count;++i)destination[i]+=source[i]*gain;
        return;
    }
    for(SampleIndex frame=0;frame<frames;++frame){
        const auto ringBase=static_cast<std::size_t>(writeFrame_)*static_cast<std::size_t>(channels_);
        const auto ioBase=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
        for(int channel=0;channel<channels_;++channel){
            const auto c=static_cast<std::size_t>(channel);
            const float delayed=storage_[ringBase+c];
            storage_[ringBase+c]=source[ioBase+c];
            destination[ioBase+c]+=delayed*gain;
        }
        if(++writeFrame_>=delaySamples_)writeFrame_=0;
    }
}

bool RealtimePluginChain::prepare(const std::vector<PluginInstance>&plugins,
                                  const PluginHost*host,
                                  int sampleRate,
                                  int channels,
                                  SampleIndex maxBlockFrames,
                                  std::vector<RealtimePluginIssue>*issues){
    slots_.clear();latencySamples_=0;channels_=std::max(1,channels);maxBlockFrames_=std::max<SampleIndex>(1,maxBlockFrames);
    bool allPrepared=true;
    auto issue=[&](const PluginInstance&p,std::string message){allPrepared=false;if(issues)issues->push_back({p.id,std::move(message)});};

    for(auto const&p:plugins){
        if(!p.enabled||p.bypass)continue;
        Slot slot;slot.config=p;slot.builtin=p.format=="builtin";
        if(slot.builtin){slots_.push_back(std::move(slot));continue;}
        if(!host){issue(p,"No external plugin host is attached to the realtime engine");continue;}

        std::string error;auto processor=host->createProcessor(p,error);
        if(!processor){issue(p,error.empty()?"External processor could not be created":error);continue;}
        if(!processor->supportsRealtimeProcessing()){issue(p,"Processor backend does not expose callback-safe block processing");continue;}
        try{
            if(!processor->prepare(sampleRate,channels_,error)){issue(p,error.empty()?"Processor prepare failed":error);continue;}
            processor->setState(p.opaqueState);
        }catch(const std::exception&e){issue(p,std::string("Processor preparation/state restore threw: ")+e.what());continue;}
        catch(...){issue(p,"Processor preparation/state restore threw an unknown exception");continue;}

        slot.latencySamples=std::max(0,processor->latencySamples());
        slot.processor=std::move(processor);
        const auto scratchSamples=static_cast<std::size_t>(maxBlockFrames_)*static_cast<std::size_t>(channels_);
        slot.inputScratch.assign(scratchSamples,0.0f);
        if(slot.latencySamples>0||std::clamp(slot.config.wet,0.0f,1.0f)<1.0f)slot.delayedDryScratch.assign(scratchSamples,0.0f);
        slot.dryDelay.prepare(slot.latencySamples,channels_);
        const long long sum=static_cast<long long>(latencySamples_)+slot.latencySamples;
        latencySamples_=static_cast<int>(std::min<long long>(sum,std::numeric_limits<int>::max()));
        slots_.push_back(std::move(slot));
    }
    return allPrepared;
}

void RealtimePluginChain::process(float*interleaved,SampleIndex frames) noexcept{
    if(!interleaved||frames<=0)return;
    for(auto&slot:slots_){
        if(slot.builtin){
            for(SampleIndex frame=0;frame<frames;++frame){
                auto*sample=interleaved+static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
                if(channels_==1){float right=sample[0];processRealtimeBuiltinSample(slot.config,sample[0],right);}
                else processRealtimeBuiltinSample(slot.config,sample[0],sample[1]);
            }
            continue;
        }
        if(!slot.processor)continue;
        const float wet=std::clamp(slot.config.wet,0.0f,1.0f);
        SampleIndex offset=0;
        while(offset<frames){
            const SampleIndex chunk=std::min(maxBlockFrames_,frames-offset);
            const auto sampleCount=static_cast<std::size_t>(chunk)*static_cast<std::size_t>(channels_);
            float*block=interleaved+static_cast<std::size_t>(offset)*static_cast<std::size_t>(channels_);
            std::copy_n(block,sampleCount,slot.inputScratch.data());
            const bool processed=slot.processor->processRealtime(block,chunk,channels_);
            if(!processed){
                if(slot.latencySamples>0){
                    std::fill_n(slot.delayedDryScratch.data(),sampleCount,0.0f);
                    slot.dryDelay.addDelayed(slot.inputScratch.data(),slot.delayedDryScratch.data(),chunk);
                    std::copy_n(slot.delayedDryScratch.data(),sampleCount,block);
                }else std::copy_n(slot.inputScratch.data(),sampleCount,block);
            }else if(wet<1.0f){
                std::fill_n(slot.delayedDryScratch.data(),sampleCount,0.0f);
                slot.dryDelay.addDelayed(slot.inputScratch.data(),slot.delayedDryScratch.data(),chunk);
                for(std::size_t i=0;i<sampleCount;++i)block[i]=slot.delayedDryScratch[i]*(1.0f-wet)+block[i]*wet;
            }
            offset+=chunk;
        }
    }
}

void RealtimePluginChain::reset() noexcept{
    for(auto&slot:slots_){slot.dryDelay.reset();if(slot.processor)slot.processor->resetRealtime();}
}

} // namespace flowdaw
