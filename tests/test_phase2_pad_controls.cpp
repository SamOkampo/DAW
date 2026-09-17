#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Serialization.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;

static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static double channelMeanAbs(const AudioBuffer&a,SampleIndex begin,SampleIndex end,int ch){
    begin=std::max<SampleIndex>(0,begin);end=std::min<SampleIndex>(a.frames(),end);double sum=0.0;SampleIndex n=0;
    for(SampleIndex i=begin;i<end;++i){sum+=std::abs(a.interleaved[static_cast<std::size_t>(i*a.channels+ch)]);++n;}
    return n?sum/static_cast<double>(n):0.0;
}

static std::shared_ptr<AudioBuffer> makePadAudio(){
    auto a=std::make_shared<AudioBuffer>();a->sampleRate=48000;a->channels=1;a->interleaved.resize(20000);
    for(SampleIndex i=0;i<a->frames();++i)a->interleaved[static_cast<std::size_t>(i)]=i<12000?0.9f:0.2f;
    return a;
}

int main(){
    try{
        Project p;p.name="Phase2 Pad Controls";p.transport.bpm=120.0;
        SampleAsset sample;sample.name="pad source";sample.audio=makePadAudio();
        SampleSlice a;a.name="Open";a.startFrame=0;a.endFrame=12000;a.gain=1.0f;a.pan=0.0f;a.chokeGroup=1;
        SampleSlice b;b.name="Close";b.startFrame=12000;b.endFrame=20000;b.gain=0.5f;b.pan=0.0f;b.chokeGroup=1;
        const Id sampleId=sample.id,firstSlice=a.id,secondSlice=b.id;sample.slices={a,b};p.samples.push_back(sample);

        Pattern take;take.name="Chop Take";take.stepCount=16;take.stepsPerBeat=4;
        ChopEvent e1;e1.recordedTick=0;e1.tick=0;e1.sampleId=sampleId;e1.sliceId=firstSlice;
        ChopEvent e2;e2.recordedTick=240;e2.tick=240;e2.sampleId=sampleId;e2.sliceId=secondSlice;
        take.chopEvents={e1,e2};const Id takeId=take.id;p.patterns.push_back(take);
        Track t;t.name="CHOPS";PatternPlacement pp;pp.patternId=takeId;t.patternClips.push_back(pp);p.tracks.push_back(t);

        const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase2_v10.flow";
        ProjectSerializer::save(p,path);auto loaded=ProjectSerializer::load(path,false);
        require(loaded.formatVersion==10,"project must migrate/save as v10");
        require(loaded.samples.size()==1&&loaded.samples[0].slices.size()==2,"v10 slices must survive save/load");
        require(std::abs(loaded.samples[0].slices[1].gain-0.5f)<0.0001f,"slice gain must persist");
        require(loaded.samples[0].slices[0].chokeGroup==1&&loaded.samples[0].slices[1].chokeGroup==1,"slice choke group must persist");
        std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");

        const auto legacyPath=std::filesystem::temp_directory_path()/"flowdaw_phase2_v6.flow";
        {
            std::ofstream f(legacyPath);
            f<<"FLOWDAW_PROJECT 6\nNAME \"legacy\"\nSAMPLE_RATE 48000\nBPM 90\nPLAYHEAD 0\nMASTER 1 0\n";
            f<<"SAMPLES 1\nSAMPLE 1 \"sample\" \"\" \"\" 90 0.8 0 1 1\nSLICE 2 \"slice\" 0 100\nTRACKS 0\nPATTERNS 0\nEND\n";
        }
        auto legacy=ProjectSerializer::load(legacyPath,false);std::filesystem::remove(legacyPath);
        require(legacy.formatVersion==10,"v6 project must migrate to v10 in memory");
        require(std::abs(legacy.samples[0].slices[0].gain-1.0f)<0.0001f&&legacy.samples[0].slices[0].chokeGroup==0,"v6 slice defaults must be safe");

        AudioEngine preview;
        auto audio=makePadAudio();
        require(preview.triggerPreview(audio,0,12000,1.0f,-1.0f,1),"first preview command");
        auto leftBlock=preview.renderDeviceBlockForTest(128);
        require(channelMeanAbs(leftBlock,0,128,0)>0.5&&channelMeanAbs(leftBlock,0,128,1)<0.01,"first pad should be hard-left");
        require(preview.triggerPreview(audio,12000,4000,1.0f,1.0f,1),"second preview command");
        auto rightBlock=preview.renderDeviceBlockForTest(128);
        require(channelMeanAbs(rightBlock,0,128,1)>0.1&&channelMeanAbs(rightBlock,0,128,0)<0.01,"same choke group must stop previous preview voice");

        AudioEngine engine;engine.publish(p);auto rendered=engine.renderOffline(9000);
        require(channelMeanAbs(rendered,500,1500,0)>0.45,"first chop should render before choke point");
        require(channelMeanAbs(rendered,6500,7500,0)<0.2,"second same-group chop must choke the first in Arrangement render");

        std::cout<<"FLOWDAW Phase 2 pad controls tests: PASS\n";return 0;
    }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
