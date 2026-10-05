#include "flowdaw/CoreLibrary.hpp"
#include "flowdaw/ContentLibrary.hpp"
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
    manifest<<"LIBRARY_VERSION 1\n";

    std::ofstream provenance(root/"PROVENANCE.txt",std::ios::trunc);
    if(!provenance)throw std::runtime_error("Could not write FLOW Core provenance");
    provenance<<"FLOWDAW Core Library — provenance\n";
    provenance<<"All audio in this library is deterministically synthesized by FLOWDAW source code.\n";
    provenance<<"No third-party recordings, commercial sample packs or externally copyrighted audio are used.\n";
    provenance<<"Generator: flowdaw::writeFlowCoreLibrary, library version 1.\n\n";

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
    manifest<<"END\n";
    manifest.close();
    provenance.close();

    const auto parsed=loadContentManifest(root/"flow-core.manifest");
    if(parsed.entries.size()!=specs().size())throw std::runtime_error("FLOW Core manifest self-check failed");

    return {specs().size(),sampleRate};
}

}
