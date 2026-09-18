#include "flowdaw/RealtimeRouting.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace flowdaw;

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static bool near(float a,float b,float eps=0.0001f){return std::abs(a-b)<=eps;}

static void testTopologyPdc(){
    RealtimeTrackLatencyInput direct;
    direct.trackId=1;
    direct.pluginLatencySamples=0;
    direct.outputBusId=0;
    direct.sends.push_back({101,10,true});

    RealtimeTrackLatencyInput busTrack;
    busTrack.trackId=2;
    busTrack.pluginLatencySamples=64;
    busTrack.outputBusId=10;

    RealtimeTrackLatencyInput secondBusTrack;
    secondBusTrack.trackId=3;
    secondBusTrack.pluginLatencySamples=16;
    secondBusTrack.outputBusId=20;

    auto plan=buildRealtimePdcPlan(
        {direct,busTrack,secondBusTrack},
        {{10,128},{20,32}});

    require(plan.issues.empty(),"valid topology unexpectedly produced PDC issues");
    require(plan.preMasterLatencySamples==192,"pre-master latency should follow longest track->bus->master path");

    auto const*t1=plan.findTrack(1);
    auto const*t2=plan.findTrack(2);
    auto const*t3=plan.findTrack(3);
    auto const*b10=plan.findBus(10);
    auto const*b20=plan.findBus(20);
    require(t1&&t2&&t3&&b10&&b20,"PDC plan lost a route");

    require(t1->outputsToMaster&&t1->outputDelaySamples==192,"direct track was not aligned to longest master path");
    require(t1->sends.size()==1&&t1->sends[0].delaySamples==64,"send was not aligned to destination bus input latency");
    require(!t2->outputsToMaster&&t2->outputDelaySamples==0,"longest bus input should require no pre-bus delay");
    require(!t3->outputsToMaster&&t3->outputDelaySamples==0,"single-source bus should require no pre-bus delay");

    require(b10->inputLatencySamples==64&&b10->totalLatencySamples==192&&b10->masterDelaySamples==0,
            "primary bus latency accounting is incorrect");
    require(b20->inputLatencySamples==16&&b20->totalLatencySamples==48&&b20->masterDelaySamples==144,
            "shorter bus was not delayed to master alignment");
}

static void testInvalidRoutes(){
    RealtimeTrackLatencyInput track;
    track.trackId=7;
    track.pluginLatencySamples=11;
    track.outputBusId=999;
    track.sends.push_back({701,888,true});
    auto plan=buildRealtimePdcPlan({track},{{20,5}});

    require(plan.issues.size()==2,"invalid output/send routes should be reported");
    require(plan.preMasterLatencySamples==11,"missing output bus should fall back to direct master latency");
    auto const*t=plan.findTrack(7);
    require(t&&t->outputsToMaster&&t->outputDelaySamples==0,"missing output bus did not fall back to master");
    require(t->sends.empty(),"invalid send should not enter executable PDC plan");
}

static void testRouteBuffer(){
    RealtimeStereoRouteBuffer buffer;
    buffer.prepare(4);
    require(buffer.capacityFrames()==4,"route buffer capacity mismatch");
    require(buffer.clear(4),"route buffer failed valid clear");
    float source[]={1,2,3,4,5,6,7,8};
    require(buffer.addFrom(source,4,0.5f),"route buffer failed valid add");
    for(int i=0;i<8;++i)require(near(buffer.data()[i],source[i]*0.5f),"route buffer summing mismatch");
    require(!buffer.clear(5),"route buffer accepted a block larger than prepared capacity");
}

int main(){
    try{
        testTopologyPdc();
        testInvalidRoutes();
        testRouteBuffer();
        std::cout<<"Phase 8 routing/PDC planner OK\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"Phase 8 routing/PDC planner failed: "<<e.what()<<"\n";
        return 1;
    }
}
