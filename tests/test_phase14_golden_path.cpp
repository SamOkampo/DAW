#include "flowdaw/Mastering.hpp"
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
static NativeEffectPreset preset(const std::string&id){
    const auto all=builtinNativeEffectPresets();
    const auto it=std::find_if(all.begin(),all.end(),[&](const auto&p){return p.id==id;});
    if(it==all.end())throw std::runtime_error("missing mastering golden preset: "+id);
    return *it;
}
static double energy(const AudioBuffer&b){double e=0.0;for(float x:b.interleaved)e+=static_cast<double>(x)*x;return e;}

int main(){
    try{
        const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase14_golden";
        std::filesystem::remove_all(root);std::filesystem::create_directories(root);

        AudioBuffer source;source.sampleRate=48000;source.channels=2;source.interleaved.resize(48000*6*2);
        constexpr double pi=3.14159265358979323846;
        for(int f=0;f<48000*6;++f){
            const float body=0.035f*static_cast<float>(std::sin(2.0*pi*220.0*f/48000.0))
                           +0.018f*static_cast<float>(std::sin(2.0*pi*880.0*f/48000.0));
            const float transient=(f%24000<48)?0.035f*(1.0f-static_cast<float>(f%24000)/48.0f):0.0f;
            source.interleaved[static_cast<std::size_t>(f)*2]=body+transient;
            source.interleaved[static_cast<std::size_t>(f)*2+1]=body*0.88f-transient*0.25f;
        }
        const auto sourcePath=root/"source.wav";WavFile::writeFloat32(sourcePath,source);

        Project project;project.name="Phase 14 Mastering Golden";project.sampleRate=48000;project.transport.bpm=120.0;
        SampleAsset sample;sample.name="Mastering Source";sample.path=sourcePath;sample.audio=std::make_shared<AudioBuffer>(source);const Id sid=sample.id;project.samples.push_back(sample);
        Track track;track.name="MIX";Clip clip;clip.sampleId=sid;clip.sourceLength=source.frames();clip.lengthTicks=MusicalTime::samplesToTicks(source.frames(),project.transport.bpm,project.sampleRate);track.clips.push_back(clip);
        for(const auto&id:{"eq-clean","compressor-glue","saturator-warm"})track.mixer.plugins.push_back(instantiateNativeEffectPreset(preset(id)));
        project.tracks.push_back(track);
        project.master.plugins.push_back(instantiateNativeEffectPreset(preset("limiter-safe")));

        const auto projectPath=root/"MasteringGolden.flow";ProjectSerializer::save(project,projectPath);
        auto reopened=ProjectSerializer::load(projectPath,true);
        require(reopened.formatVersion==11,"Phase 14 golden project must remain .flow v11");

        const auto rendered=renderProjectOffline(reopened,0.0);
        require(rendered.frames()>=source.frames()-1&&energy(rendered)>1.0,"Phase 14 production render failed");
        const auto before=analyzeMastering(rendered);
        require(std::isfinite(before.integratedLufs)&&std::isfinite(before.maxTruePeakDbTP),"Phase 14 pre-export analysis invalid");

        MasterExportOptions options;options.bitDepth=MasterBitDepth::PCM24;options.dither=DitherMode::TPDF;options.normalizeLoudness=true;
        options.target.targetLufs=-23.0;options.target.maxTruePeakDbTP=-1.0;options.target.toleranceLu=0.2;
        const auto output=root/"MasteringGolden_R128.wav";
        const auto report=exportMasteredWav(reopened,output,options,0.0);
        require(std::filesystem::is_regular_file(output),"Phase 14 mastered WAV missing");
        require(report.normalized,"Phase 14 golden export did not execute normalization policy");
        require(report.delivered.maxTruePeakDbTP<=-0.94,"Phase 14 golden report exceeded true-peak ceiling");
        require(report.targetWithinTolerance||report.limitedByTruePeak,"Phase 14 golden export must either meet loudness target or explicitly report TP constraint");

        const auto delivered=WavFile::read(output);
        require(delivered.sampleRate==48000&&delivered.channels==2&&delivered.frames()==rendered.frames(),"Phase 14 PCM24 delivery format/frame regression");
        require(energy(delivered)>1.0,"Phase 14 mastered delivery is silent");
        const auto measured=analyzeMastering(delivered);
        require(std::abs(measured.maxTruePeakDbTP-report.delivered.maxTruePeakDbTP)<0.08,"decoded 24-bit true peak diverged from export report");
        require(std::abs(measured.integratedLufs-report.delivered.integratedLufs)<0.08,"decoded 24-bit LUFS diverged from export report");
        require(measured.maxTruePeakDbTP<=-0.90,"decoded 24-bit master exceeded delivery ceiling");
        if(!report.limitedByTruePeak)require(std::abs(measured.integratedLufs+23.0)<=0.25,"decoded 24-bit master missed EBU loudness target");

        std::filesystem::remove_all(root);
        std::cout<<"FLOWDAW Phase 14.7 mastering golden path: PASS\n";return 0;
    }catch(const std::exception&e){
        std::cerr<<"FLOWDAW Phase 14.7 mastering golden path: FAIL: "<<e.what()<<"\n";return 1;
    }
}
