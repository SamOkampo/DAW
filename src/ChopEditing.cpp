#include "flowdaw/ChopEditing.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace flowdaw {
namespace {
std::uint64_t mix64(std::uint64_t x){
    x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;
}
double signedUnit(Id id,std::uint64_t salt){
    const auto h=mix64(static_cast<std::uint64_t>(id)^salt);
    return (static_cast<double>(h&0xFFFFFFULL)/static_cast<double>(0xFFFFFFULL))*2.0-1.0;
}
void captureLegacyUiEvents(Pattern& pattern){
    if(pattern.chopQuantizeStrength!=0.0f||pattern.chopHumanize!=0.0f)return;
    for(auto&ev:pattern.chopEvents){
        if(ev.recordedTick==0&&ev.tick!=0){ev.recordedTick=ev.tick;ev.recordedVelocity=ev.velocity;}
    }
}
}

void applyChopEditing(Pattern& pattern){
    const Tick grid=std::max<Tick>(1,pattern.chopQuantizeGridTicks);
    const float strength=std::clamp(pattern.chopQuantizeStrength,0.0f,1.0f);
    const float human=std::clamp(pattern.chopHumanize,0.0f,1.0f);
    for(auto&ev:pattern.chopEvents){
        const Tick nearest=static_cast<Tick>(std::llround(static_cast<double>(ev.recordedTick)/static_cast<double>(grid)))*grid;
        const Tick delta=nearest-ev.recordedTick;
        const Tick quantized=ev.recordedTick+static_cast<Tick>(std::llround(static_cast<double>(delta)*strength));
        const Tick humanTicks=static_cast<Tick>(std::llround(signedUnit(ev.id,0xC401ULL)*human*static_cast<double>(grid)*0.12));
        ev.tick=std::max<Tick>(0,quantized+humanTicks);
        const float velocityDelta=static_cast<float>(signedUnit(ev.id,0xC402ULL))*human*0.12f;
        ev.velocity=std::clamp(ev.recordedVelocity+velocityDelta,0.0f,1.5f);
    }
}

void quantizeChopEvents(Pattern& pattern,Tick gridTicks,float strength){
    captureLegacyUiEvents(pattern);
    pattern.chopQuantizeGridTicks=std::max<Tick>(1,gridTicks);
    pattern.chopQuantizeStrength=std::clamp(strength,0.0f,1.0f);
    applyChopEditing(pattern);
}

void humanizeChopEvents(Pattern& pattern,float amount){
    captureLegacyUiEvents(pattern);
    pattern.chopHumanize=std::clamp(amount,0.0f,1.0f);
    applyChopEditing(pattern);
}

void resetChopEditing(Pattern& pattern){
    captureLegacyUiEvents(pattern);
    pattern.chopQuantizeStrength=0.0f;
    pattern.chopHumanize=0.0f;
    applyChopEditing(pattern);
}
}
