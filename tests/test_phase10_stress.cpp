#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Project.hpp"
#include "flowdaw/Wav.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace flowdaw;

static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}

static Project makeStressProject(){
    Project p;
    p.transport.bpm=120.0;
    SampleAsset sample;
    sample.name="Phase10 stress tone";
    sample.audio=std::make_shared<AudioBuffer>();
    sample.audio->sampleRate=48000;
    sample.audio->channels=1;
    sample.audio->interleaved.resize(2048);
    for(std::size_t i=0;i<sample.audio->interleaved.size();++i)
        sample.audio->interleaved[i]=0.15f*std::sin(static_cast<float>(i)*0.05f);
    const Id sampleId=sample.id;
    p.samples.push_back(sample);

    Track track;
    track.name="Stress Track";
    Clip clip;
    clip.sampleId=sampleId;
    clip.sourceLength=sample.audio->frames();
    clip.lengthTicks=kPPQ*4;
    track.clips.push_back(clip);
    p.tracks.push_back(track);
    return p;
}

static void testRepeatedPublishRender(){
    AudioEngine engine;
    engine.configureExternalDevice(48000,64,false);
    auto project=makeStressProject();
    engine.publish(project);
    engine.play();

    constexpr int iterations=500;
    const auto started=std::chrono::steady_clock::now();
    for(int i=0;i<iterations;++i){
        project.master.volume=0.75f+0.001f*static_cast<float>(i%100);
        project.tracks[0].mixer.volume=0.80f+0.001f*static_cast<float>(i%80);
        engine.publish(project);
        auto block=engine.renderDeviceBlockForTest(64);
        require(block.frames()==64,"stress render changed block size");
        for(float sample:block.interleaved)require(std::isfinite(sample),"stress render produced non-finite audio");
        require(engine.collectRetiredGraphs()==0,"idle publish/render stress accumulated retired graphs");
    }
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
    std::cout<<"Phase 10 publish/render baseline: "<<iterations<<" iterations in "<<elapsed<<" ms\n";
}

static void testPreviewQueueStress(){
    AudioEngine engine;
    engine.configureExternalDevice(48000,64,false);
    auto audio=std::make_shared<AudioBuffer>();
    audio->sampleRate=48000;
    audio->channels=1;
    audio->interleaved.resize(1024,0.2f);

    int accepted=0,rejected=0;
    for(int i=0;i<128;++i){
        if(engine.triggerPreview(audio,0,audio->frames(),0.5f,0.0f,0))++accepted;
        else ++rejected;
    }
    require(accepted>0,"preview stress accepted no commands");
    require(rejected>0,"preview queue did not bound saturated producer input");

    auto block=engine.renderDeviceBlockForTest(256);
    require(block.frames()==256,"preview stress render changed block size");
    for(float sample:block.interleaved)require(std::isfinite(sample),"preview stress produced non-finite audio");

    engine.stopPreviews();
    (void)engine.renderDeviceBlockForTest(64);
    require(engine.triggerPreview(audio,0,audio->frames(),0.5f,0.0f,0),"preview queue did not recover after callback consumption");
}

int main(){
    try{
        testRepeatedPublishRender();
        testPreviewQueueStress();
        std::cout<<"Phase 10 performance/robustness regression OK\n";
        return 0;
    }catch(const std::exception&e){
        std::cerr<<"Phase 10 performance/robustness regression failed: "<<e.what()<<"\n";
        return 1;
    }
}
