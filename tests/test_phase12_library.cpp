#include "flowdaw/ContentLibrary.hpp"
#include "flowdaw/CoreLibrary.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

static std::string readText(const std::filesystem::path& path){
    std::ifstream in(path);
    std::ostringstream out;
    out<<in.rdbuf();
    return out.str();
}

int main(){
    try{
        const auto temp=std::filesystem::temp_directory_path();
        const auto root=temp/"flowdaw_phase12_core_library";
        const auto second=temp/"flowdaw_phase12_core_library_repeat";
        std::filesystem::remove_all(root);
        std::filesystem::remove_all(second);

        const auto summary=writeFlowCoreLibrary(root,48000);
        require(summary.sampleCount==12,"FLOW Core must contain the initial twelve curated samples");
        require(summary.sampleRate==48000,"FLOW Core generation sample rate");

        const auto manifest=loadContentManifest(root/"flow-core.manifest");
        require(manifest.libraryId=="flow.core","FLOW Core stable library id");
        require(manifest.libraryVersion==2,"FLOW Core library version advances when native presets are added");

        const std::set<std::string> expectedCategories{
            "Drums/Kicks","Drums/Snares-Claps","Drums/Hats","Drums/Percussion","Drums/808s","FX"
        };
        std::set<std::string> categories;
        std::set<std::string> ids;
        const auto provenance=readText(root/"PROVENANCE.txt");
        require(provenance.find("No third-party recordings")!=std::string::npos,"FLOW Core provenance statement");

        std::size_t sampleEntries=0;
        for(const auto& entry:manifest.entries){
            require(ids.insert(entry.id).second,"FLOW Core content IDs unique");
            if(entry.kind!=ContentKind::Sample)continue;
            ++sampleEntries;
            categories.insert(entry.category);
            require(provenance.find(entry.id)!=std::string::npos,"every FLOW Core sample id has provenance");
            const auto path=resolveContentPath(root,entry);
            require(std::filesystem::is_regular_file(path),"FLOW Core WAV exists");
            const auto audio=WavFile::read(path);
            require(audio.sampleRate==48000&&audio.channels==1,"FLOW Core WAV format");
            require(audio.frames()>1000,"FLOW Core WAV is non-empty");
            float peak=0.0f;
            for(const float x:audio.interleaved){
                require(std::isfinite(x),"FLOW Core WAV samples finite");
                peak=std::max(peak,std::abs(x));
            }
            require(peak>0.1f&&peak<=1.0f,"FLOW Core WAV has bounded audible signal");
        }
        require(sampleEntries==12,"FLOW Core preserves the twelve Phase 12.2 samples");
        require(categories==expectedCategories,"FLOW Core covers all 12.2 categories");

        writeFlowCoreLibrary(second,48000);
        const auto firstA=WavFile::read(root/manifest.entries.front().relativePath);
        const auto firstB=WavFile::read(second/manifest.entries.front().relativePath);
        require(firstA.interleaved==firstB.interleaved,"FLOW Core generation is deterministic");

        std::filesystem::remove_all(root);
        std::filesystem::remove_all(second);
        std::cout<<"FLOWDAW Phase 12.2 core library tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12.2 core library tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
