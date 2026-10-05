#include "flowdaw/CoreLibrary.hpp"
#include "flowdaw/FirstPartyContent.hpp"
#include "flowdaw/ProjectTemplates.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/NativeDrums.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

static std::string readAll(const std::filesystem::path& path){
    std::ifstream in(path);
    std::ostringstream out;
    out<<in.rdbuf();
    return out.str();
}

static const Pattern* instrumentPattern(const Project& p){
    for(const auto& pattern:p.patterns)if(pattern.instrument.enabled)return &pattern;
    return nullptr;
}

int main(){
    try{
        const auto temp=std::filesystem::temp_directory_path();
        const auto root=temp/"flowdaw_phase12_template_core";
        std::filesystem::remove_all(root);
        const auto summary=writeFlowCoreLibrary(root,48000);
        require(summary.sampleCount==12&&summary.presetCount==10,"FLOW Core prerequisites");

        struct Case{
            ProjectTemplateKind kind;
            double bpm;
            const char* name;
            const char* firstSampleId;
            const char* instrumentType;
        };
        const Case cases[]={
            {ProjectTemplateKind::BoomBap,90.0,"Boom Bap Starter","flow.kick.deep","flow_keys"},
            {ProjectTemplateKind::Trap,140.0,"Trap Starter","flow.kick.punch","flow_808"},
            {ProjectTemplateKind::LoFi,82.0,"Lo-Fi Starter","flow.kick.deep","flow_keys"}
        };

        for(const auto& tc:cases){
            auto p=makeProjectTemplate(tc.kind,root);
            require(p.formatVersion==11,"template stays .flow v11");
            require(p.name==tc.name,"template name");
            require(std::abs(p.transport.bpm-tc.bpm)<0.001,"template bpm");
            require(p.samples.size()==3,"template uses three bundled drum samples");
            require(p.patterns.size()>=2,"template has drum and instrument patterns");

            for(const auto& sample:p.samples){
                require(sample.path.empty(),"bundled sample path stays empty");
                require(isFirstPartyContentKey(sample.nativeKey),"bundled sample uses content: stable reference");
                require(sample.audio&&sample.audio->frames()>1000,"bundled sample is hydrated in template");
            }
            require(firstPartyContentId(p.samples.front().nativeKey)==tc.firstSampleId,"template selects expected kick");

            const auto* instrument=instrumentPattern(p);
            require(instrument!=nullptr,"template has native instrument pattern");
            require(instrument->instrument.type==tc.instrumentType,"template applies expected native preset engine");
            require(!instrument->midiNotes.empty(),"template instrument pattern is immediately audible");

            const auto projectPath=temp/(std::string("flowdaw_phase12_")+tc.name+".flow");
            ProjectSerializer::save(p,projectPath);
            const auto serialized=readAll(projectPath);
            require(serialized.find(root.string())==std::string::npos,"serialized project does not contain FLOW Core absolute root");
            require(serialized.find("content:flow.")!=std::string::npos,"serialized project preserves stable content references");

            auto loaded=ProjectSerializer::load(projectPath,true);
            require(loaded.formatVersion==11,"roundtrip stays v11");
            require(loaded.samples.size()==3,"roundtrip preserves bundled sample structure");
            for(const auto& sample:loaded.samples){
                require(isFirstPartyContentKey(sample.nativeKey),"roundtrip preserves content reference");
                require(!sample.audio,"content refs remain loadable when no resolver is supplied");
            }

            const auto hydrated=hydrateFirstPartyContent(loaded,root);
            require(hydrated==3,"resolver hydrates all bundled source samples");
            for(const auto& sample:loaded.samples)require(sample.audio&&sample.audio->frames()>1000,"hydrated bundled sample audio");

            std::filesystem::remove(projectPath);
            std::filesystem::remove(projectPath.string()+".bak");
        }

        const auto blank=makeProjectTemplate(ProjectTemplateKind::Blank,root);
        require(blank.formatVersion==11&&blank.samples.empty(),"blank template needs no bundled content");
        require(std::abs(blank.transport.bpm-120.0)<0.001,"blank template bpm");

        Project legacy;
        legacy.name="Legacy Native Drum";
        SampleAsset kick;
        kick.name="Legacy Kick";
        kick.nativeKey="kick";
        kick.audio=std::make_shared<AudioBuffer>(makeNativeDrum("kick",legacy.sampleRate));
        legacy.samples.push_back(kick);
        const auto legacyPath=temp/"flowdaw_phase12_legacy_native.flow";
        ProjectSerializer::save(legacy,legacyPath);
        auto loadedLegacy=ProjectSerializer::load(legacyPath,true);
        require(loadedLegacy.samples.size()==1&&loadedLegacy.samples[0].audio&&loadedLegacy.samples[0].audio->frames()>0,"legacy native drum projects still reload");
        require(loadedLegacy.formatVersion==11,"legacy native roundtrip normalizes to v11");

        std::filesystem::remove(legacyPath);
        std::filesystem::remove(legacyPath.string()+".bak");
        std::filesystem::remove_all(root);

        std::cout<<"FLOWDAW Phase 12.5 starter template tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12.5 starter template tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
