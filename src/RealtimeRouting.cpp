#include "flowdaw/RealtimeRouting.hpp"
#include <algorithm>
#include <unordered_map>

namespace flowdaw {

const RealtimeTrackPdcPlan* RealtimePdcPlan::findTrack(Id id) const noexcept {
    auto it=std::find_if(tracks.begin(),tracks.end(),[&](auto const& p){return p.trackId==id;});
    return it==tracks.end()?nullptr:&*it;
}

const RealtimeBusPdcPlan* RealtimePdcPlan::findBus(Id id) const noexcept {
    auto it=std::find_if(buses.begin(),buses.end(),[&](auto const& p){return p.busId==id;});
    return it==buses.end()?nullptr:&*it;
}

RealtimePdcPlan buildRealtimePdcPlan(const std::vector<RealtimeTrackLatencyInput>& tracks,
                                     const std::vector<RealtimeBusLatencyInput>& buses) {
    RealtimePdcPlan plan;
    plan.buses.reserve(buses.size());
    plan.tracks.reserve(tracks.size());

    std::unordered_map<Id,std::size_t> busIndex;
    busIndex.reserve(buses.size());
    for(auto const& input:buses) {
        if(input.busId==0 || busIndex.contains(input.busId)) {
            plan.issues.push_back({input.busId,"Duplicate/invalid bus id ignored by realtime PDC planner"});
            continue;
        }
        busIndex.emplace(input.busId,plan.buses.size());
        RealtimeBusPdcPlan bus;
        bus.busId=input.busId;
        bus.pluginLatencySamples=std::max(0,input.pluginLatencySamples);
        plan.buses.push_back(bus);
    }

    int directMasterLatency=0;
    auto noteBusInput=[&](Id busId,int sourceLatency,Id routeId,const char* kind) {
        auto it=busIndex.find(busId);
        if(it==busIndex.end()) {
            plan.issues.push_back({routeId,std::string(kind)+" references an unknown bus"});
            return false;
        }
        auto& bus=plan.buses[it->second];
        bus.active=true;
        bus.inputLatencySamples=std::max(bus.inputLatencySamples,std::max(0,sourceLatency));
        return true;
    };

    for(auto const& track:tracks) {
        const int latency=std::max(0,track.pluginLatencySamples);
        if(track.outputBusId==0) directMasterLatency=std::max(directMasterLatency,latency);
        else if(!noteBusInput(track.outputBusId,latency,track.trackId,"Track output")) {
            // Match the AudioEngine's historical behavior: a missing output bus
            // falls back to the master rather than dropping the track.
            directMasterLatency=std::max(directMasterLatency,latency);
        }

        for(auto const& send:track.sends) {
            if(!send.enabled) continue;
            if(send.busId==0) {
                plan.issues.push_back({send.sendId,"Enabled send has no destination bus"});
                continue;
            }
            (void)noteBusInput(send.busId,latency,send.sendId,"Send");
        }
    }

    int preMaster=directMasterLatency;
    for(auto& bus:plan.buses) {
        if(!bus.active) continue;
        bus.totalLatencySamples=bus.inputLatencySamples+bus.pluginLatencySamples;
        preMaster=std::max(preMaster,bus.totalLatencySamples);
    }
    plan.preMasterLatencySamples=preMaster;

    for(auto& bus:plan.buses) {
        if(bus.active) bus.masterDelaySamples=std::max(0,preMaster-bus.totalLatencySamples);
    }

    for(auto const& track:tracks) {
        RealtimeTrackPdcPlan out;
        out.trackId=track.trackId;
        out.pluginLatencySamples=std::max(0,track.pluginLatencySamples);
        out.outputBusId=track.outputBusId;

        auto busIt=track.outputBusId==0?busIndex.end():busIndex.find(track.outputBusId);
        if(track.outputBusId==0 || busIt==busIndex.end()) {
            out.outputsToMaster=true;
            out.outputDelaySamples=std::max(0,preMaster-out.pluginLatencySamples);
        } else {
            out.outputsToMaster=false;
            auto const& bus=plan.buses[busIt->second];
            out.outputDelaySamples=std::max(0,bus.inputLatencySamples-out.pluginLatencySamples);
        }

        for(auto const& send:track.sends) {
            if(!send.enabled || send.busId==0) continue;
            auto it=busIndex.find(send.busId);
            if(it==busIndex.end()) continue;
            RealtimeSendPdcPlan sendPlan;
            sendPlan.sendId=send.sendId;
            sendPlan.busId=send.busId;
            sendPlan.delaySamples=std::max(0,plan.buses[it->second].inputLatencySamples-out.pluginLatencySamples);
            out.sends.push_back(sendPlan);
        }
        plan.tracks.push_back(std::move(out));
    }

    return plan;
}

void RealtimeStereoRouteBuffer::prepare(SampleIndex maxFrames) {
    capacityFrames_=std::max<SampleIndex>(0,maxFrames);
    activeFrames_=0;
    storage_.assign(static_cast<std::size_t>(capacityFrames_)*2U,0.0f);
}

bool RealtimeStereoRouteBuffer::clear(SampleIndex frames) noexcept {
    if(frames<0 || frames>capacityFrames_) return false;
    activeFrames_=frames;
    std::fill_n(storage_.data(),static_cast<std::size_t>(frames)*2U,0.0f);
    return true;
}

bool RealtimeStereoRouteBuffer::addFrom(const float* source,SampleIndex frames,float gain) noexcept {
    if(!source || frames<0 || frames>activeFrames_) return false;
    const auto count=static_cast<std::size_t>(frames)*2U;
    for(std::size_t i=0;i<count;++i) storage_[i]+=source[i]*gain;
    return true;
}

} // namespace flowdaw
