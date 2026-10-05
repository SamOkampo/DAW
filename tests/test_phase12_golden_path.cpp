#include "flowdaw/ContentAudit.hpp"
#include "flowdaw/ContentDiscovery.hpp"
#include "flowdaw/CoreLibrary.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/FirstPartyContent.hpp"
#include "flowdaw/ProjectTemplates.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Wav.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

static double energy(const AudioBuffer& audio){
    double total=0.0;
    for(const float x:audio.interleaved)total+=std::abs(x);
    return total;
}

static std::string readAll(const std::filesystem::path& path){
    std::ifstream in(path);
    std::ostringstream out;
    out<<in.rdbuf();
    return out.str();
}

int main(){
    try{
        const auto temp=std::filesystem::temp_directory_path()/"flowdaw_phase12_golden";
        std::filesystem::remove_all(temp);

        const auto prefix=temp/"install";
        const auto coreRoot=prefix/"share"/"FLOWDAW"/"content"/"flow-core";
        const auto generated=writeFlowCoreLibrary(coreRoot,48000);
        require(generated.sampleCount==12&&generated.presetCount==10,"golden path FLOW Core generation");

        const auto executable=prefix/"bin"/"FLOWDAW";
        const auto discovered=discoverFlowCoreRoot(executable);
        require(discovered==coreRoot.lexically_normal(),"golden path installed FLOW Core discovery");

        const auto audit=auditFlowCoreLibrary(discovered);
        require(audit.passed,"golden path content integrity audit");
        require(audit.sampleCount==12&&audit.presetCount==10,"golden path content counts");

        auto project=makeProjectTemplate(ProjectTemplateKind::BoomBap,discovered);
        require(project.formatVersion==11,"golden path template stays v11");
        require(project.samples.size()==3,"golden path starter uses FLOW Core samples");
        for(const auto& sample:project.samples){
            require(sample.path.empty(),"golden path bundled sample does not persist install path");
            require(isFirstPartyContentKey(sample.nativeKey),"golden path bundled sample uses stable content reference");
            require(sample.audio&&sample.audio->frames()>1000,"golden path bundled sample initially hydrated");
        }

        const auto initialRender=renderProjectOffline(project,0.0);
        require(initialRender.frames()>0&&energy(initialRender)>10.0,"golden path starter renders audible audio");

        const auto projectPath=temp/"BoomBap.flow";
        ProjectSerializer::save(project,projectPath);
        const auto serialized=readAll(projectPath);
        require(serialized.find(prefix.string())==std::string::npos,"golden path .flow excludes install prefix");
        require(serialized.find("content:flow.")!=std::string::npos,"golden path .flow keeps stable first-party IDs");

        auto reopened=ProjectSerializer::load(projectPath,true);
        require(reopened.formatVersion==11,"golden path reopen stays v11");
        for(const auto& sample:reopened.samples)require(!sample.audio,"golden path reopen remains resolver-independent before hydration");

        const auto hydrated=hydrateFirstPartyContent(reopened,discovered);
        require(hydrated==3,"golden path reopen hydrates bundled samples");
        for(const auto& sample:reopened.samples)require(sample.audio&&sample.audio->frames()>1000,"golden path reopened samples are audible");

        const auto reopenedRender=renderProjectOffline(reopened,0.0);
        require(reopenedRender.frames()==initialRender.frames(),"golden path render length stable after reopen");
        require(energy(reopenedRender)>10.0,"golden path reopened project renders audible audio");

        const auto exportPath=temp/"BoomBap-export.wav";
        exportProjectWav(reopened,exportPath,0.0);
        require(std::filesystem::is_regular_file(exportPath),"golden path master export exists");
        const auto exported=WavFile::read(exportPath);
        require(exported.frames()>0&&energy(exported)>10.0,"golden path exported WAV is audible");

        std::filesystem::remove_all(temp);
        std::cout<<"FLOWDAW Phase 12.8 native-content golden path: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12.8 native-content golden path: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
