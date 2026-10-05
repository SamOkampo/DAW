#include "flowdaw/ContentDiscovery.hpp"
#include "flowdaw/ContentLibrary.hpp"
#include <vector>

namespace flowdaw {

bool isValidFlowCoreRoot(const std::filesystem::path& root){
    if(root.empty())return false;
    std::error_code ec;
    const auto manifestPath=root/"flow-core.manifest";
    const auto provenancePath=root/"PROVENANCE.txt";
    if(!std::filesystem::is_regular_file(manifestPath,ec))return false;
    ec.clear();
    if(!std::filesystem::is_regular_file(provenancePath,ec))return false;
    try{
        const auto manifest=loadContentManifest(manifestPath);
        return manifest.libraryId=="flow.core"&&!manifest.entries.empty();
    }catch(...){
        return false;
    }
}

std::filesystem::path discoverFlowCoreRoot(
    const std::filesystem::path& executablePath,
    const std::filesystem::path& buildFallback){
    std::vector<std::filesystem::path> candidates;
    if(!executablePath.empty()){
        const auto exeDir=executablePath.parent_path();
        if(!exeDir.empty()){
            const auto prefix=exeDir.parent_path();
            if(!prefix.empty())candidates.push_back(prefix/"share"/"FLOWDAW"/"content"/"flow-core");
            if(!prefix.empty())candidates.push_back(prefix/"Resources"/"FLOWDAW"/"content"/"flow-core");
            candidates.push_back(exeDir/"content"/"flow-core");
        }
    }
    if(!buildFallback.empty())candidates.push_back(buildFallback);

    for(const auto& candidate:candidates){
        if(isValidFlowCoreRoot(candidate))return candidate.lexically_normal();
    }
    return {};
}

}
