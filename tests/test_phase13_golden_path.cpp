#include "flowdaw/Export.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/NativePresets.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static double energy(const AudioBuffer&b){double e=0.0;for(float x:b.interleaved)e+=static_cast<double>(x)*x;return e;}
static float peak(const AudioBuffer&b){float p=0.0f;for(float x:b.interleaved){require(std::isfinite(x),"golden path non-finite audio");p=std::max(p,std::abs(x));}return p;}
static NativeEffectPreset preset(const std::string&id){
    const auto presets=builtinNativeEffectPresets();
    auto it=std::find_if(presets.begin(),presets.end(),[&](const auto&p){return p.id==id;});
    if(it==presets.end())throw std::runtime_error("missing golden-path preset: "+id);
    return *it;
}

int main(){
    try{
        const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase13_golden";
        std::filesystem::remove_all(root);std::filesystem::create_directories(root);

        AudioBuffer source;source.sampleRate=48000;source.channels=2;source.interleaved.resize(48000*2);
        constexpr double pi=3.14159265358979323846;
        for(int f=0;f<48000;++f){
            const float kick=0.42f*std::sin(2.0*pi*(72.0+28.0*std::exp(-f/3500.0))*f/48000.0)*static_cast<float>(std::exp(-f/10000.0));
            const float tone=0.12f*static_cast<float>(std::sin(2.0*pi*440.0*f/48000.0));
            const float transient=(f%12000<32)?0.18f*(1.0f-static_cast<float>(f%12000)/32.0f):0.0f;
            const float l=kick+tone+transient,r=kick+tone*0.82f-transient*0.35f;
            source.interleaved[static_cast<std::size_t>(f)*2]=l;
            source.interleaved[static_cast<std::size_t>(f)*2+1]=r;
        }
        const auto sourcePath=root/"source.wav";WavFile::writeFloat32(sourcePath,source);

        Project project;project.name="Phase 13 Native Golden Path";project.sampleRate=48000;project.transport.bpm=120.0;
        SampleAsset sample;sample.name="Native Golden Source";sample.path=sourcePath;sample.audio=std::make_shared<AudioBuffer>(source);const Id sid=sample.id;project.samples.push_back(sample);
        Track track;track.name="NATIVE MIX";Clip clip;clip.sampleId=sid;clip.sourceLength=source.frames();clip.lengthTicks=MusicalTime::samplesToTicks(source.frames(),project.transport.bpm,project.sampleRate);track.clips.push_back(clip);

        for(const auto&id:{"eq-vocal-presence","compressor-glue","saturator-warm","chorus-wide","gate-tight","utility-wide"})
            track.mixer.plugins.push_back(instantiateNativeEffectPreset(preset(id)));
        project.tracks.push_back(track);
        for(const auto&id:{"delay-eighth","reverb-small-room","limiter-safe"})
            project.master.plugins.push_back(instantiateNativeEffectPreset(preset(id)));

        const auto projectPath=root/"NativeGolden.flow";ProjectSerializer::save(project,projectPath);
        auto reopened=ProjectSerializer::load(projectPath,true);
        require(reopened.formatVersion==11,"golden path project must remain .flow v11");
        require(reopened.tracks.size()==1&&reopened.tracks[0].mixer.plugins.size()==6,"golden path track native chain persistence");
        require(reopened.master.plugins.size()==3,"golden path master native chain persistence");

        const auto rendered=renderProjectOffline(reopened,2.0);
        require(rendered.frames()>source.frames(),"golden path tail render missing");
        require(energy(rendered)>5.0,"golden path native chain rendered silence");
        const float renderedPeak=peak(rendered);
        require(renderedPeak<=1.01f,"golden path limiter did not bound sample peaks");

        const auto exportPath=root/"NativeGolden.wav";exportProjectWav(reopened,exportPath,2.0);
        require(std::filesystem::is_regular_file(exportPath),"golden path WAV export missing");
        const auto exported=WavFile::read(exportPath);
        require(exported.sampleRate==48000&&exported.channels==2,"golden path WAV format changed");
        require(exported.frames()==rendered.frames(),"golden path export frame count mismatch");
        require(energy(exported)>5.0,"golden path exported WAV is silent");
        require(peak(exported)<=1.01f,"golden path exported WAV peak escaped limiter");

        std::filesystem::remove_all(root);
        std::cout<<"FLOWDAW Phase 13.13 native-only golden path: PASS\n";return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 13.13 native-only golden path: FAIL: "<<e.what()<<"\n";return 1;
    }
}
