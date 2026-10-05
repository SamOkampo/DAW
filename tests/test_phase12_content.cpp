#include "flowdaw/ContentLibrary.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

static std::filesystem::path writeManifest(const std::filesystem::path& root,const std::string& body,const char* name){
    const auto path=root/name;
    std::ofstream out(path,std::ios::trunc);
    out<<body;
    return path;
}

int main(){
    try{
        const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase12_content";
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);

        const auto valid=writeManifest(root,
            "FLOWDAW_CONTENT 1\n"
            "LIBRARY_ID \"flow.core\"\n"
            "DISPLAY_NAME \"FLOW Core Library\"\n"
            "LIBRARY_VERSION 1\n"
            "ENTRY \"flow.kick.01\" SAMPLE \"Drums/Kicks/flow_kick_01.wav\" \"Drums/Kicks\" \"dark,punchy\"\n"
            "ENTRY \"flow.keys.dark\" INSTRUMENT_PRESET \"Presets/Keys/dark.flowpreset\" \"Keys\" \"dark,warm\"\n"
            "END\n","valid.manifest");

        const auto manifest=loadContentManifest(valid);
        require(manifest.schemaVersion==1,"content schema version");
        require(manifest.libraryId=="flow.core","library id");
        require(manifest.displayName=="FLOW Core Library","display name");
        require(manifest.entries.size()==2,"content entries parsed");
        require(manifest.entries[0].kind==ContentKind::Sample,"sample kind parsed");
        require(manifest.entries[1].kind==ContentKind::InstrumentPreset,"preset kind parsed");
        require(manifest.entries[0].tags.size()==2,"tags parsed");
        require(resolveContentPath(root,manifest.entries[0])==root/"Drums/Kicks/flow_kick_01.wav","sample path resolves below root");

        require(isSafeContentRelativePath("Drums/Kicks/kick.wav"),"normal relative path accepted");
        require(!isSafeContentRelativePath("../escape.wav"),"parent traversal rejected");
        require(!isSafeContentRelativePath(std::filesystem::path("/tmp/escape.wav")),"absolute path rejected");

        bool duplicateRejected=false;
        try{
            const auto duplicate=writeManifest(root,
                "FLOWDAW_CONTENT 1\nLIBRARY_ID \"flow.core\"\nDISPLAY_NAME \"FLOW Core Library\"\nLIBRARY_VERSION 1\n"
                "ENTRY \"same\" SAMPLE \"a.wav\" \"Drums\" \"\"\n"
                "ENTRY \"same\" SAMPLE \"b.wav\" \"Drums\" \"\"\nEND\n","duplicate.manifest");
            (void)loadContentManifest(duplicate);
        }catch(const std::exception&){duplicateRejected=true;}
        require(duplicateRejected,"duplicate ids rejected");

        bool traversalRejected=false;
        try{
            const auto traversal=writeManifest(root,
                "FLOWDAW_CONTENT 1\nLIBRARY_ID \"flow.core\"\nDISPLAY_NAME \"FLOW Core Library\"\nLIBRARY_VERSION 1\n"
                "ENTRY \"escape\" SAMPLE \"../escape.wav\" \"Drums\" \"\"\nEND\n","traversal.manifest");
            (void)loadContentManifest(traversal);
        }catch(const std::exception&){traversalRejected=true;}
        require(traversalRejected,"manifest traversal rejected");

        bool extensionRejected=false;
        try{
            const auto invalidExtension=writeManifest(root,
                "FLOWDAW_CONTENT 1\nLIBRARY_ID \"flow.core\"\nDISPLAY_NAME \"FLOW Core Library\"\nLIBRARY_VERSION 1\n"
                "ENTRY \"bad\" SAMPLE \"bad.mp3\" \"Drums\" \"\"\nEND\n","extension.manifest");
            (void)loadContentManifest(invalidExtension);
        }catch(const std::exception&){extensionRejected=true;}
        require(extensionRejected,"unsupported native sample format rejected");

        std::filesystem::remove_all(root);
        std::cout<<"FLOWDAW Phase 12 content library tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12 content library tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
