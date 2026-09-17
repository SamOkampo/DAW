#include "flowdaw/Automation.hpp"
#include <algorithm>

namespace flowdaw {
void normalizeAutomationLane(AutomationLane& lane){
    std::sort(lane.points.begin(),lane.points.end(),[](auto const&a,auto const&b){return a.tick<b.tick;});
    std::vector<AutomationPoint> out;out.reserve(lane.points.size());
    for(auto p:lane.points){p.tick=std::max<Tick>(0,p.tick);if(!out.empty()&&out.back().tick==p.tick)out.back()=p;else out.push_back(p);}lane.points=std::move(out);
}
float automationValueAt(const AutomationLane& lane,Tick tick,float fallback){
    if(lane.points.empty())return fallback;
    if(tick<=lane.points.front().tick)return lane.points.front().value;
    if(tick>=lane.points.back().tick)return lane.points.back().value;
    auto it=std::upper_bound(lane.points.begin(),lane.points.end(),tick,[](Tick t,const AutomationPoint&p){return t<p.tick;});
    if(it==lane.points.begin())return it->value;auto const&b=*it;auto const&a=*(it-1);const auto span=std::max<Tick>(1,b.tick-a.tick);const float u=static_cast<float>(tick-a.tick)/static_cast<float>(span);return a.value+(b.value-a.value)*u;
}
const AutomationLane* findAutomation(const Project&project,const std::string&target,Id targetId,Id subTargetId){
    for(auto const&lane:project.automation)if(lane.target==target&&lane.targetId==targetId&&lane.subTargetId==subTargetId)return &lane;return nullptr;
}
}
