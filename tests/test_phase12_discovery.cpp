#include "flowdaw/ContentDiscovery.hpp"
#include "flowdaw/CoreLibrary.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

int main(){
    try{
        const auto temp=std::filesystem::temp_directory_path()/"flowdaw_phase12_discovery";
        std::filesystem::remove_all(temp);

        const auto linuxRoot=temp/"linux"/"share"/"FLOWDAW"/"content"/"flow-core";
        writeFlowCoreLibrary(linuxRoot,48000);
        const auto linuxExe=temp/"linux"/"bin"/"FLOWDAW";
        require(discoverFlowCoreRoot(linuxExe)==linuxRoot.lexically_normal(),"linux/windows prefix discovery");

        const auto macRoot=temp/"mac"/"FLOWDAW.app"/"Contents"/"Resources"/"FLOWDAW"/"content"/"flow-core";
        writeFlowCoreLibrary(macRoot,48000);
        const auto macExe=temp/"mac"/"FLOWDAW.app"/"Contents"/"MacOS"/"FLOWDAW";
        require(discoverFlowCoreRoot(macExe)==macRoot.lexically_normal(),"macOS app bundle discovery");

        const auto fallback=temp/"build"/"flow-core";
        writeFlowCoreLibrary(fallback,48000);
        require(discoverFlowCoreRoot(temp/"missing"/"bin"/"FLOWDAW",fallback)==fallback.lexically_normal(),"build fallback discovery");

        require(isValidFlowCoreRoot(linuxRoot),"valid FLOW Core root accepted");
        const auto corrupt=temp/"corrupt";
        std::filesystem::create_directories(corrupt);
        std::ofstream(corrupt/"flow-core.manifest")<<"not a manifest\n";
        std::ofstream(corrupt/"PROVENANCE.txt")<<"test\n";
        require(!isValidFlowCoreRoot(corrupt),"corrupt manifest rejected");
        require(discoverFlowCoreRoot(temp/"none"/"bin"/"FLOWDAW").empty(),"missing content returns empty path");

        std::filesystem::remove_all(temp);
        std::cout<<"FLOWDAW Phase 12.6 content discovery tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12.6 content discovery tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
