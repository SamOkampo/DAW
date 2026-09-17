#pragma once
#include "flowdaw/Project.hpp"
#include <string>
#include <vector>

namespace flowdaw {
struct ProjectHealthIssue {
    std::string code;
    std::string message;
    Id targetId=0;
    bool error=false;
};
struct WorkflowCommand {
    std::string id;
    std::string title;
    std::string description;
};

std::vector<ProjectHealthIssue> validateProject(const Project& project);
std::vector<WorkflowCommand> workflowCommands(const std::string& query={});
bool executeWorkflowCommand(Project& project,const std::string& commandId,Id targetId,std::string& result);
}
