#include "flowdaw/ContentAudit.hpp"
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
        const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase12_integrity";
        std::filesystem::remove_all(root);
        writeFlowCoreLibrary(root,48000);

        auto report=auditFlowCoreLibrary(root);
        require(report.passed,"generated FLOW Core passes integrity audit");
        require(report.sampleCount==12,"integrity audit sees twelve samples");
        require(report.presetCount==10,"integrity audit sees ten presets");

        const auto missing=root/"Drums/Kicks/flow_kick_deep.wav";
        std::filesystem::remove(missing);
        report=auditFlowCoreLibrary(root);
        require(!report.passed,"missing manifest asset fails audit");

        std::filesystem::remove_all(root);
        writeFlowCoreLibrary(root,48000);
        std::filesystem::create_directories(root/"rogue");
        std::ofstream(root/"rogue/undeclared.wav",std::ios::binary)<<"not-wave";
        report=auditFlowCoreLibrary(root);
        require(!report.passed,"undeclared WAV fails audit");

        std::filesystem::remove_all(root);
        writeFlowCoreLibrary(root,48000);
        std::ofstream(root/"Presets/Keys/dark-keys.flowpreset",std::ios::trunc)
            <<"FLOWDAW_INSTRUMENT_PRESET 1\nID \"wrong.id\"\nNAME \"Broken\"\nCATEGORY \"Instruments/Keys\"\nTYPE \"flow_keys\"\nGAIN 0.8\nPAN 0\nATTACK_MS 5\nRELEASE_MS 100\nTONE 0.5\nDRIVE 0\nDELAY_MIX 0\nDELAY_TICKS 480\nEND\n";
        report=auditFlowCoreLibrary(root);
        require(!report.passed,"preset identity mismatch fails audit");

        std::filesystem::remove_all(root);
        writeFlowCoreLibrary(root,48000);
        std::ofstream(root/"PROVENANCE.txt",std::ios::trunc)<<"incomplete\n";
        report=auditFlowCoreLibrary(root);
        require(!report.passed,"incomplete provenance fails audit");

        std::filesystem::remove_all(root);
        std::cout<<"FLOWDAW Phase 12.7 content integrity audit tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12.7 content integrity audit tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
