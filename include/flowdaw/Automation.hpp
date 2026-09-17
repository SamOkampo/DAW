#pragma once
#include "flowdaw/Project.hpp"

namespace flowdaw {
float automationValueAt(const AutomationLane& lane, Tick tick, float fallback);
const AutomationLane* findAutomation(const Project& project,const std::string& target,Id targetId,Id subTargetId=0);
void normalizeAutomationLane(AutomationLane& lane);
}
