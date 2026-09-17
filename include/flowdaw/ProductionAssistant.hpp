#pragma once
#include "flowdaw/Project.hpp"
#include <string>
#include <vector>

namespace flowdaw {
enum class AssistantAction {
    None,
    SetMasterHeadroom,
    CreateMixBus,
    TrimTrack,
    AddMasterSoftClip,
    AddTrackGain,
    GroovePattern
};
struct AssistantSuggestion {
    std::string key;
    std::string title;
    std::string detail;
    std::string actionLabel;
    AssistantAction action=AssistantAction::None;
    Id targetId=0;
    float value=0.0f;
    int priority=0;
};

std::string buildProjectContext(const Project& project);
std::vector<AssistantSuggestion> analyzeProductionContext(const Project& project);
bool applyAssistantSuggestion(Project& project,const AssistantSuggestion& suggestion,std::string& result);
}
