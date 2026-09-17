#include "flowdaw/ChopEditing.hpp"
#include <algorithm>
#include <cmath>

namespace flowdaw {
void quantizeChopEvents(Pattern& pattern,Tick gridTicks,float strength){
    if(gridTicks<=0||pattern.chopEvents.empty())return;
    strength=std::clamp(strength,0.0f,1.0f);
    for(auto&ev:pattern.chopEvents){
        const auto nearest=static_cast<Tick>(std::llround(static_cast<double>(ev.tick)/static_cast<double>(gridTicks)))*gridTicks;
        const auto delta=nearest-ev.tick;
        ev.tick=std::max<Tick>(0,ev.tick+static_cast<Tick>(std::llround(static_cast<double>(delta)*strength)));
    }
}
}
