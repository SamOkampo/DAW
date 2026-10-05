#include "flowdaw/CoreLibrary.hpp"
#include "flowdaw/ContentLibrary.hpp"
#include "flowdaw/NativePresets.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

namespace flowdaw {
namespace {
constexpr double pi=3.14159265358979323846;

struct Spec {
    const char* id;
    const char* relativePath;
    const char* category;
    const char* tags;
};

const std::vector<Spec>& specs(){
    static const std::vector<Spec> value{
        {"flow.kick.deep","Drums/Kicks/flow_kick_deep.wav","Drums/Kicks","deep,warm,boom-bap"},
        {"flow.kick.punch","Drums/Kicks/flow_kick_punch.wav","Drums/Kicks","punchy,tight,trap"},
        {"flow.snare.dust","Drums/Snares-Claps/flow_snare_dust.wav","Drums/Snares-Claps","dusty,snare,boom-bap"},
        {"flow.clap.snap","Drums/Snares-Claps/flow_clap_snap.wav","Drums/Snares-Claps","clap,snap,wide"},
        {"flow.hat.tight","Drums/Hats/flow_hat_tight.wav","Drums/Hats","closed,hat,tight"},
        {"flow.hat.open","Drums/Hats/flow_hat_open.wav","Drums/Hats","open,hat,bright"},
        {"flow.perc.rim","Drums/Percussion/flow_perc_rim.wav","Drums/Percussion","rim,click,perc"},
        {"flow.perc.wood","Drums/Percussion/flow_perc_wood.wav","Drums/Percussion","wood,tonal,perc"},
        {"flow.808.sub","Drums/808s/flow_808_sub.wav","Drums/808s","808,sub,clean"},
        {"flow.808.drive","Drums/808s/flow_808_drive.wav","Drums/808s","808,drive,trap"},
        {"flow.fx.riser","FX/flow_fx_riser.wav","FX","riser,transition,noise"},
        {"flow.fx.impact","FX/flow_fx_impact.wav","FX","impact,transition,low"}
    };
    return value;
}


struct PresetSpec {
    const char* id;
    const char* name;
    const char* relativePath;
    const char* category;
    const char* tags;
    const char* type;
    float gain;
    float pan;
    float attackMs;
    float releaseMs;
    float tone;
    float drive;
    float delayMix;
    Tick delayTicks;
};

const std::vector<PresetSpec>& presetSpecs(){
    static const std::vector<PresetSpec> value{
        {"flow.preset.keys.dark","Dark Keys","Presets/Keys/dark-keys.flowpreset","Instruments/Keys","dark,warm,keys","flow_keys",0.78f,0.0f,8.0f,420.0f,0.22f,0.10f,0.13f,kPPQ/4},
        {"flow.preset.keys.glass","Glass Keys","Presets/Keys/glass-keys.flowpreset","Instruments/Keys","glass,bright,keys","flow_keys",0.72f,0.0f,4.0f,600.0f,0.82f,0.0f,0.24f,kPPQ/2},
        {"flow.preset.bass.warm","Warm Bass","Presets/Bass/warm-bass.flowpreset","Instruments/Bass","warm,sub,bass","flow_bass",0.88f,0.0f,6.0f,300.0f,0.28f,0.08f,0.0f,kPPQ/2},
        {"flow.preset.bass.grit","Grit Bass","Presets/Bass/grit-bass.flowpreset","Instruments/Bass","grit,bass,drive","flow_bass",0.80f,0.0f,3.0f,220.0f,0.72f,0.32f,0.06f,kPPQ/4},
        {"flow.preset.808.clean","Clean 808","Presets/808/clean-808.flowpreset","Instruments/808","808,sub,clean","flow_808",0.90f,0.0f,1.0f,900.0f,0.22f,0.02f,0.0f,kPPQ/2},
        {"flow.preset.808.dirty","Dirty 808","Presets/808/dirty-808.flowpreset","Instruments/808","808,dirty,drive","flow_808",0.82f,0.0f,1.0f,1100.0f,0.58f,0.38f,0.0f,kPPQ/2},
        {"flow.preset.lead.neon","Neon Lead","Presets/Lead/neon-lead.flowpreset","Instruments/Lead","lead,bright,delay","flow_lead",0.68f,0.0f,2.0f,260.0f,0.82f,0.14f,0.22f,kPPQ/4},
        {"flow.preset.lead.soft","Soft Lead","Presets/Lead/soft-lead.flowpreset","Instruments/Lead","lead,soft,melodic","flow_lead",0.64f,0.0f,18.0f,500.0f,0.42f,0.04f,0.18f,kPPQ/2},
        {"flow.preset.pad.dream","Dream Pad","Presets/Pad/dream-pad.flowpreset","Instruments/Pad","pad,dream,ambient","flow_keys",0.62f,0.0f,520.0f,2200.0f,0.64f,0.02f,0.28f,kPPQ/2},
        {"flow.preset.pad.dust","Dust Pad","Presets/Pad/dust-pad.flowpreset","Instruments/Pad","pad,dust,lo-fi","flow_keys",0.58f,0.0f,340.0f,1800.0f,0.24f,0.18f,0.20f,kPPQ/2}
    };
    return value;
}

NativeInstrumentPreset makePreset(const PresetSpec& spec){
    NativeInstrumentPreset preset;
    preset.id=spec.id;
    preset.name=spec.name;
    preset.category=spec.category;
    preset.instrument.enabled=true;
    preset.instrument.type=spec.type;
    preset.instrument.gain=spec.gain;
    preset.instrument.pan=spec.pan;
    preset.instrument.attackMs=spec.attackMs;
    preset.instrument.releaseMs=spec.releaseMs;
    preset.instrument.tone=spec.tone;
    preset.instrument.drive=spec.drive;
    preset.instrument.delayMix=spec.delayMix;
    preset.instrument.delayTicks=spec.delayTicks;
    return preset;
}

AudioBuffer mono(int sampleRate,double seconds){
    AudioBuffer out;
    out.sampleRate=sampleRate;
    out.channels=1;
    out.interleaved.assign(static_cast<std::size_t>(std::max(1.0,std::round(seconds*sampleRate))),0.0f);
    return out;
}

void normalize(AudioBuffer& audio,float peakTarget=0.92f){
    float peak=0.0f;
    for(const float x:audio.interleaved)peak=std::max(peak,std::abs(x));
    if(peak<=0.000001f)return;
    const float gain=peakTarget/peak;
    for(auto& x:audio.interleaved)x=std::clamp(x*gain,-1.0f,1.0f);
}

float deterministicNoise(std::uint32_t& state){
    state=state*1664525u+1013904223u;
    return (static_cast<float>((state>>9)&0x7fffff)/4194303.5f)-1.0f;
}

AudioBuffer kick(int sampleRate,double startHz,double endHz,double decay,float click){
    auto out=mono(sampleRate,0.55);
    double phase=0.0;
    std::uint32_t rng=0x12f4a981u;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const double freq=endHz+(startHz-endHz)*std::exp(-t*28.0);
        phase+=2.0*pi*freq/sampleRate;
        const float body=static_cast<float>(std::sin(phase)*std::exp(-t*decay));
        const float transient=deterministicNoise(rng)*static_cast<float>(std::exp(-t*105.0))*static_cast<float>(click);
        out.interleaved[i]=body+transient;
    }
    normalize(out);
    return out;
}

AudioBuffer snare(int sampleRate,bool clap){
    auto out=mono(sampleRate,clap?0.42:0.36);
    std::uint32_t rng=clap?0x9917a41du:0x6712bc3fu;
    float previous=0.0f;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const float n=deterministicNoise(rng);
        const float hp=n-previous*0.74f;
        previous=n;
        if(clap){
            const double local=std::fmod(t,0.019);
            const double burst=(t<0.085?1.0:0.42)*std::exp(-local*92.0);
            out.interleaved[i]=hp*static_cast<float>(burst*std::exp(-t*9.5));
        }else{
            const float tone=static_cast<float>(0.28*std::sin(2.0*pi*188.0*t));
            out.interleaved[i]=(0.78f*hp+tone)*static_cast<float>(std::exp(-t*14.0));
        }
    }
    normalize(out,0.88f);
    return out;
}

AudioBuffer hat(int sampleRate,bool open){
    auto out=mono(sampleRate,open?0.58:0.16);
    std::uint32_t rng=open?0x28b3f119u:0x4d21c777u;
    float previous=0.0f;
    const double decay=open?8.5:38.0;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const float n=deterministicNoise(rng);
        const float hp=n-previous*0.82f;
        previous=n;
        const float metal=0.68f*hp
            +static_cast<float>(0.18*std::sin(2.0*pi*7350.0*t))
            +static_cast<float>(0.10*std::sin(2.0*pi*10300.0*t));
        out.interleaved[i]=metal*static_cast<float>(std::exp(-t*decay));
    }
    normalize(out,0.72f);
    return out;
}

AudioBuffer percussion(int sampleRate,bool rim){
    auto out=mono(sampleRate,rim?0.17:0.28);
    std::uint32_t rng=rim?0x7129ad43u:0xa71c2d13u;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const float n=deterministicNoise(rng);
        if(rim){
            out.interleaved[i]=static_cast<float>(
                (0.74*std::sin(2.0*pi*1760.0*t)+0.18*std::sin(2.0*pi*2480.0*t)+0.08*n)
                *std::exp(-t*35.0));
        }else{
            out.interleaved[i]=static_cast<float>(
                (0.52*std::sin(2.0*pi*430.0*t)+0.34*std::sin(2.0*pi*690.0*t)+0.14*n)
                *std::exp(-t*17.0));
        }
    }
    normalize(out,0.82f);
    return out;
}

AudioBuffer eightOhEight(int sampleRate,bool driven){
    auto out=mono(sampleRate,1.35);
    double phase=0.0;
    const double base=driven?51.91:43.65;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const double freq=base*(1.0+0.30*std::exp(-t*21.0));
        phase+=2.0*pi*freq/sampleRate;
        const float env=static_cast<float>(std::exp(-t*(driven?2.9:2.35)));
        float x=static_cast<float>(std::sin(phase))*env;
        if(driven)x=std::tanh(x*2.8f)*0.92f;
        out.interleaved[i]=x;
    }
    normalize(out,0.9f);
    return out;
}

AudioBuffer riser(int sampleRate){
    auto out=mono(sampleRate,1.6);
    std::uint32_t rng=0x8b3721d9u;
    float previous=0.0f;
    double phase=0.0;
    const double duration=static_cast<double>(out.interleaved.size())/sampleRate;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const double u=std::clamp(t/duration,0.0,1.0);
        const double freq=180.0+4200.0*u*u;
        phase+=2.0*pi*freq/sampleRate;
        const float n=deterministicNoise(rng);
        const float hp=n-previous*0.8f;
        previous=n;
        const float envelope=static_cast<float>(u*u*std::min(1.0,(1.0-u)*12.0));
        out.interleaved[i]=(0.58f*hp+0.42f*static_cast<float>(std::sin(phase)))*envelope;
    }
    normalize(out,0.86f);
    return out;
}

AudioBuffer impact(int sampleRate){
    auto out=mono(sampleRate,1.1);
    std::uint32_t rng=0x44f20b71u;
    double phase=0.0;
    for(std::size_t i=0;i<out.interleaved.size();++i){
        const double t=static_cast<double>(i)/sampleRate;
        const double freq=72.0+54.0*std::exp(-t*18.0);
        phase+=2.0*pi*freq/sampleRate;
        const float low=static_cast<float>(std::sin(phase)*std::exp(-t*4.5));
        const float transient=deterministicNoise(rng)*static_cast<float>(std::exp(-t*28.0));
        out.interleaved[i]=0.82f*low+0.28f*transient;
    }
    normalize(out,0.91f);
    return out;
}

AudioBuffer generate(const std::string& id,int sampleRate){
    if(id=="flow.kick.deep")return kick(sampleRate,132.0,43.0,7.7,0.08);
    if(id=="flow.kick.punch")return kick(sampleRate,176.0,52.0,10.2,0.20);
    if(id=="flow.snare.dust")return snare(sampleRate,false);
    if(id=="flow.clap.snap")return snare(sampleRate,true);
    if(id=="flow.hat.tight")return hat(sampleRate,false);
    if(id=="flow.hat.open")return hat(sampleRate,true);
    if(id=="flow.perc.rim")return percussion(sampleRate,true);
    if(id=="flow.perc.wood")return percussion(sampleRate,false);
    if(id=="flow.808.sub")return eightOhEight(sampleRate,false);
    if(id=="flow.808.drive")return eightOhEight(sampleRate,true);
    if(id=="flow.fx.riser")return riser(sampleRate);
    if(id=="flow.fx.impact")return impact(sampleRate);
    throw std::invalid_argument("Unknown FLOW Core sample id: "+id);
}
}

CoreLibraryBuildSummary writeFlowCoreLibrary(const std::filesystem::path& root,int sampleRate){
    if(sampleRate<8000||sampleRate>384000)throw std::invalid_argument("FLOW Core sample rate out of range");
    std::filesystem::create_directories(root);

    std::ofstream manifest(root/"flow-core.manifest",std::ios::trunc);
    if(!manifest)throw std::runtime_error("Could not write FLOW Core manifest");
    manifest<<"FLOWDAW_CONTENT 1\n";
    manifest<<"LIBRARY_ID "<<std::quoted("flow.core")<<"\n";
    manifest<<"DISPLAY_NAME "<<std::quoted("FLOW Core Library")<<"\n";
    manifest<<"LIBRARY_VERSION 2\n";

    std::ofstream provenance(root/"PROVENANCE.txt",std::ios::trunc);
    if(!provenance)throw std::runtime_error("Could not write FLOW Core provenance");
    provenance<<"FLOWDAW Core Library — provenance\n";
    provenance<<"All audio in this library is deterministically synthesized by FLOWDAW source code.\n";
    provenance<<"No third-party recordings, commercial sample packs or externally copyrighted audio are used.\n";
    provenance<<"Generator: flowdaw::writeFlowCoreLibrary, library version 2.\n";
    provenance<<"Native preset files contain first-party parameter metadata only.\n\n";

    std::ofstream rights(root/"CONTENT_RIGHTS.txt",std::ios::trunc);
    if(!rights)throw std::runtime_error("Could not write FLOW Core rights notice");
    rights<<"FLOWDAW Core Library — content rights / provenance notice\n";
    rights<<"Origin: first-party deterministic synthesis and first-party preset metadata generated by FLOWDAW source code.\n";
    rights<<"No third-party audio recordings or commercial sample-pack assets are included in FLOW Core v2.\n";
    rights<<"This notice records provenance and does not grant a public license.\n";
    rights<<"Distribution terms for FLOWDAW software/content are defined separately by the product owner and applicable dependency licences.\n";
    rights<<"JUCE and other third-party software remain subject to their own licensing terms.\n";

    for(const auto& spec:specs()){
        const std::filesystem::path relative(spec.relativePath);
        if(!isSafeContentRelativePath(relative))throw std::runtime_error("Unsafe built-in FLOW Core path");
        const auto path=root/relative;
        std::filesystem::create_directories(path.parent_path());
        auto audio=generate(spec.id,sampleRate);
        WavFile::writeFloat32(path,audio);
        manifest<<"ENTRY "<<std::quoted(spec.id)<<" SAMPLE "<<std::quoted(relative.generic_string())
                <<" "<<std::quoted(spec.category)<<" "<<std::quoted(spec.tags)<<"\n";
        provenance<<spec.id<<"\tGENERATED\t"<<relative.generic_string()<<"\n";
    }

    for(const auto& spec:presetSpecs()){
        const std::filesystem::path relative(spec.relativePath);
        if(!isSafeContentRelativePath(relative))throw std::runtime_error("Unsafe built-in FLOW preset path");
        const auto path=root/relative;
        saveNativeInstrumentPreset(makePreset(spec),path);
        manifest<<"ENTRY "<<std::quoted(spec.id)<<" INSTRUMENT_PRESET "<<std::quoted(relative.generic_string())
                <<" "<<std::quoted(spec.category)<<" "<<std::quoted(spec.tags)<<"\n";
        provenance<<spec.id<<"\tGENERATED_PRESET\t"<<relative.generic_string()<<"\n";
    }
    manifest<<"END\n";
    manifest.close();
    provenance.close();
    rights.close();

    const auto parsed=loadContentManifest(root/"flow-core.manifest");
    if(parsed.entries.size()!=specs().size()+presetSpecs().size())throw std::runtime_error("FLOW Core manifest self-check failed");

    return {specs().size(),presetSpecs().size(),sampleRate};
}

}
