#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>

namespace flowdaw {

enum class ProjectTemplateKind {
    Blank,
    BoomBap,
    Trap,
    LoFi
};

Project makeProjectTemplate(ProjectTemplateKind kind,const std::filesystem::path& firstPartyRoot={});

}
