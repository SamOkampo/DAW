#include "flowdaw/ChopEditing.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}

static ChopEvent eventAt(Tick tick,float velocity=1.0f){
    ChopEvent e;e.recordedTick=tick;e.tick=tick;e.recordedVelocity=velocity;e.velocity=velocity;return e;
}

int main(){
    try{
        Pattern original;original.chopEvents={eventAt(100,.8f),eventAt(350,1.1f)};

        auto untouched=original;quantizeChopEvents(untouched,240,0.0f);
        require(untouched.chopEvents[0].tick==100&&untouched.chopEvents[1].tick==350,"0% quantize must preserve performance");

        auto half=original;quantizeChopEvents(half,240,0.5f);
        require(half.chopEvents[0].tick==50&&half.chopEvents[1].tick==295,"50% quantize should move halfway to grid");

        auto full=original;quantizeChopEvents(full,240,1.0f);
        require(full.chopEvents[0].tick==0&&full.chopEvents[1].tick==240,"100% quantize should snap to 1/16 grid");

        quantizeChopEvents(full,240,0.0f);
        require(full.chopEvents[0].tick==100&&full.chopEvents[1].tick==350,"quantize must be reversible from recorded timing");

        auto human=original;humanizeChopEvents(human,1.0f);
        const auto humanTick0=human.chopEvents[0].tick;const auto humanVel0=human.chopEvents[0].velocity;
        require(humanTick0!=100||std::abs(humanVel0-.8f)>.0001f,"humanize must affect timing or velocity");
        auto humanAgain=original;humanizeChopEvents(humanAgain,1.0f);
        require(humanAgain.chopEvents[0].tick==humanTick0&&std::abs(humanAgain.chopEvents[0].velocity-humanVel0)<.00001f,"humanize must be deterministic");
        humanizeChopEvents(human,0.0f);
        require(human.chopEvents[0].tick==100&&std::abs(human.chopEvents[0].velocity-.8f)<.00001f,"0% humanize restores exact recorded performance");

        auto layered=original;quantizeChopEvents(layered,240,.5f);humanizeChopEvents(layered,.5f);const auto edited=layered.chopEvents[1].tick;applyChopEditing(layered);require(layered.chopEvents[1].tick==edited,"reapplying edit state must not accumulate drift");resetChopEditing(layered);require(layered.chopEvents[1].tick==350,"reset restores captured timing");

        std::cout<<"FLOWDAW reversible ChopEditing tests: PASS\n";return 0;
    }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
