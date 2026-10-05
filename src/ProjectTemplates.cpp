#include "flowdaw/ProjectTemplates.hpp"
#include "flowdaw/FirstPartyContent.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Wav.hpp"
#include <array>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <string>

namespace flowdaw {
namespace {

struct TemplateSpec {
    const char* name;
    double bpm;
    float swing;
    const char* kickId;
    const char* snareId;
    const char* hatId;
    const char* presetId;
};

TemplateSpec specFor(ProjectTemplateKind kind){
    switch(kind){
        case ProjectTemplateKind::BoomBap:
            return {"Boom Bap Starter",90.0,0.12f,"flow.kick.deep","flow.snare.dust","flow.hat.tight","flow.preset.keys.dark"};
        case ProjectTemplateKind::Trap:
            return {"Trap Starter",140.0,0.02f,"flow.kick.punch","flow.clap.snap","flow.hat.tight","flow.preset.808.dirty"};
        case ProjectTemplateKind::LoFi:
            return {"Lo-Fi Starter",82.0,0.18f,"flow.kick.deep","flow.snare.dust","flow.hat.open","flow.preset.pad.dust"};
        case ProjectTemplateKind::Blank:
            break;
    }
    throw std::invalid_argument("Blank template has no content spec");
}

SampleAsset makeContentSample(
    const std::filesystem::path& root,
    const ContentManifest& manifest,
    const std::string& id,
    const std::string& displayName){
    const auto* entry=findContentEntryById(manifest,id,ContentKind::Sample);
    if(!entry)throw std::runtime_error("Missing FLOW Core sample id: "+id);
    SampleAsset sample;
    sample.name=displayName;
    sample.nativeKey=makeFirstPartyContentKey(id);
    sample.path.clear();
    sample.audio=std::make_shared<AudioBuffer>(WavFile::read(resolveContentPath(root,*entry)));
    if(!sample.audio||sample.audio->frames()<=0)throw std::runtime_error("FLOW Core sample is empty: "+id);
    return sample;
}

void setStep(Pattern& pattern,std::size_t lane,int step,float velocity=1.0f){
    if(lane>=pattern.lanes.size())return;
    auto& steps=pattern.lanes[lane].steps;
    if(step<0||step>=static_cast<int>(steps.size()))return;
    steps[static_cast<std::size_t>(step)].active=true;
    steps[static_cast<std::size_t>(step)].velocity=velocity;
}

void addTemplateMidi(Pattern& pattern,ProjectTemplateKind kind){
    const Tick grid=kPPQ/2;
    const std::array<int,4> boom{60,63,67,70};
    const std::array<int,4> trap{36,36,39,34};
    const std::array<int,4> lofi{60,63,65,67};
    const auto& notes=kind==ProjectTemplateKind::Trap?trap:(kind==ProjectTemplateKind::LoFi?lofi:boom);
    for(std::size_t i=0;i<notes.size();++i){
        MidiNote note;
        note.startTick=static_cast<Tick>(i)*grid*2;
        note.lengthTicks=grid;
        note.pitch=notes[i];
        note.velocity=kind==ProjectTemplateKind::Trap?0.82f:0.72f;
        pattern.midiNotes.push_back(note);
    }
}

}

Project makeProjectTemplate(ProjectTemplateKind kind,const std::filesystem::path& firstPartyRoot){
    if(kind==ProjectTemplateKind::Blank){
        Project p;
        p.name="Blank Project";
        p.transport.bpm=120.0;
        Track audio;
        audio.name="Audio 1";
        p.tracks.push_back(std::move(audio));
        return p;
    }

    const auto spec=specFor(kind);
    if(firstPartyRoot.empty())throw std::runtime_error("FLOW Core root is required for native-content template");
    const auto manifest=loadContentManifest(firstPartyRoot/"flow-core.manifest");

    Project p;
    p.name=spec.name;
    p.transport.bpm=spec.bpm;

    Track audio;
    audio.name="Audio 1";
    p.tracks.push_back(std::move(audio));

    Pattern drums;
    drums.name="FLOW Core Drums";
    drums.stepCount=16;
    drums.stepsPerBeat=4;
    drums.swing=spec.swing;

    const std::array<std::pair<std::string,std::string>,3> selected{{
        {"Kick",spec.kickId},{"Snare/Clap",spec.snareId},{"Hat",spec.hatId}
    }};
    for(const auto& choice:selected){
        auto sample=makeContentSample(firstPartyRoot,manifest,choice.second,choice.first);
        const Id sid=sample.id;
        p.samples.push_back(std::move(sample));
        DrumLane lane;
        lane.name=choice.first;
        lane.sampleId=sid;
        lane.steps.resize(16);
        drums.lanes.push_back(std::move(lane));
    }

    if(kind==ProjectTemplateKind::Trap){
        for(int step:{0,6,10})setStep(drums,0,step,0.94f);
        for(int step:{4,12})setStep(drums,1,step,0.88f);
        for(int step=0;step<16;++step)setStep(drums,2,step,(step%4==0)?0.74f:0.52f);
    }else{
        for(int step:{0,8})setStep(drums,0,step,0.94f);
        for(int step:{4,12})setStep(drums,1,step,0.86f);
        for(int step=0;step<16;step+=2)setStep(drums,2,step,(step%4==0)?0.70f:0.52f);
    }

    const Id drumsId=drums.id;
    p.patterns.push_back(std::move(drums));
    Track drumTrack;
    drumTrack.name="Drums";
    PatternPlacement drumPlacement;
    drumPlacement.patternId=drumsId;
    drumPlacement.repeats=8;
    drumTrack.patternClips.push_back(drumPlacement);
    p.tracks.push_back(std::move(drumTrack));

    const auto preset=loadFirstPartyInstrumentPreset(firstPartyRoot,spec.presetId);
    Pattern melody;
    melody.name=kind==ProjectTemplateKind::Trap?"808 Pattern":"Melody";
    melody.stepCount=32;
    melody.stepsPerBeat=4;
    melody.instrument=preset.instrument;
    melody.instrument.enabled=true;
    melody.scaleRoot=0;
    melody.scaleType="minor";
    melody.midiGridTicks=kPPQ/4;
    melody.midiDefaultLengthTicks=kPPQ/2;
    addTemplateMidi(melody,kind);

    const Id melodyId=melody.id;
    p.patterns.push_back(std::move(melody));
    Track instrument;
    instrument.name=preset.name;
    PatternPlacement melodyPlacement;
    melodyPlacement.patternId=melodyId;
    melodyPlacement.repeats=4;
    instrument.patternClips.push_back(melodyPlacement);
    p.tracks.push_back(std::move(instrument));

    return p;
}

}
