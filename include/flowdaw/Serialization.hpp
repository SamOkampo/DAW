#pragma once
#include "flowdaw/Project.hpp"
#include <filesystem>
namespace flowdaw {
class ProjectSerializer {
public:
    static void save(const Project& p,const std::filesystem::path& path);
    static Project load(const std::filesystem::path& path,bool loadAudio=true);
};
}
