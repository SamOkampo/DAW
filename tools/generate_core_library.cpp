#include "flowdaw/CoreLibrary.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv){
    try{
        if(argc!=2){
            std::cerr<<"usage: flowdaw-content-generator <output-directory>\n";
            return 2;
        }
        const auto summary=flowdaw::writeFlowCoreLibrary(std::filesystem::path(argv[1]),48000);
        std::cout<<"FLOW Core Library: "<<summary.sampleCount<<" samples + "<<summary.presetCount<<" presets @ "<<summary.sampleRate<<" Hz\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOW Core Library generation failed: "<<e.what()<<"\n";
        return 1;
    }
}
