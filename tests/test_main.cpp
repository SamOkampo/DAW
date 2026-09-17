#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/NativeDrums.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Undo.hpp"
#include "flowdaw/Wav.hpp"
#include "flowdaw/Waveform.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
static AudioBuffer tone(int sr=48000,double seconds=1.0){AudioBuffer a;a.sampleRate=sr;a.channels=1;auto n=static_cast<int>(sr*seconds);a.interleaved.resize(n);for(int i=0;i<n;++i)a.interleaved[i]=0.25f*std::sin(2.0*3.141592653589793*220.0*i/sr);return a;}
static double energy(const AudioBuffer& a){double e=0;for(float x:a.interleaved)e+=std::abs(x);return e;}
static SampleIndex firstAudibleFrame(const AudioBuffer& a,float threshold=1.0e-5f){
    for(SampleIndex i=0;i<a.frames();++i){
        for(int ch=0;ch<a.channels;++ch){
            if(std::abs(a.interleaved[static_cast<std::size_t>(i*a.channels+ch)])>threshold) return i;
        }
    }
    return -1;
}

int main(){
    try{
        require(MusicalTime::toTicks({2,1,0})==3840,"bar to ticks");
        require(MusicalTime::fromTicks(4800).bar==2&&MusicalTime::fromTicks(4800).beat==2,"ticks to bar/beat");
        require(MusicalTime::ticksToSamples(3840,120,48000)==96000,"ticks to samples");
        require(MusicalTime::samplesToTicks(96000,120,48000)==3840,"samples to ticks");

        const auto tmp=std::filesystem::temp_directory_path()/"flowdaw_test.wav";auto a=tone();WavFile::writeFloat32(tmp,a);auto b=WavFile::read(tmp);require(b.sampleRate==48000&&b.channels==1&&b.frames()==a.frames(),"wav roundtrip");
        auto peaks=buildWaveform(b,100);require(peaks.size()==100&&peaks[0].first<=peaks[0].second,"waveform");

        Project p;p.name="Round Trip Beat";p.transport.bpm=90;p.sampleRate=48000;SampleAsset s;s.name="tone";s.path=tmp;s.audio=std::make_shared<AudioBuffer>(b);auto sid=s.id;p.samples.push_back(s);Track tr;tr.name="Sample";
        Effect trackFx;trackFx.type="gain";trackFx.value=0.9f;tr.mixer.effects.push_back(trackFx);Effect masterFx;masterFx.type="limiter";masterFx.value=0.8f;p.master.effects.push_back(masterFx);
        Clip c;c.sampleId=sid;c.startTick=0;c.sourceLength=b.frames();c.lengthTicks=MusicalTime::samplesToTicks(b.frames(),90,48000);tr.clips.push_back(c);p.tracks.push_back(tr);
        const auto proj=std::filesystem::temp_directory_path()/"flowdaw_test.flow";ProjectSerializer::save(p,proj);auto loaded=ProjectSerializer::load(proj);require(loaded.name==p.name&&loaded.tracks.size()==1&&loaded.samples.size()==1,"project roundtrip");require(loaded.samples[0].audio&&loaded.samples[0].audio->frames()==b.frames(),"project reloads audio");require(loaded.master.effects.size()==1&&loaded.tracks[0].mixer.effects.size()==1,"effects persist");

        UndoStack undo;auto before=p;p.tracks[0].clips[0].startTick=960;auto after=p;undo.commit(before,after,"Move clip");require(undo.undo(p)&&p.tracks[0].clips[0].startTick==0,"undo");require(undo.redo(p)&&p.tracks[0].clips[0].startTick==960,"redo");

        p=before;AudioEngine engine;engine.publish(p);auto rendered=engine.renderOffline(48000);require(energy(rendered)>10.0,"offline engine renders audible signal");const auto out=std::filesystem::temp_directory_path()/"flowdaw_render.wav";WavFile::writeFloat32(out,rendered);require(std::filesystem::file_size(out)>1000,"render wav");

        Project seq;seq.name="Sequencer Test";seq.transport.bpm=90;Track drumTrack;drumTrack.name="Drums";Pattern pat;pat.name="Pattern 1";pat.stepCount=16;pat.stepsPerBeat=4;
        for(auto const& key:{std::string("kick"),std::string("snare"),std::string("hat")}){SampleAsset ds;ds.name=key;ds.nativeKey=key;ds.audio=std::make_shared<AudioBuffer>(makeNativeDrum(key,seq.sampleRate));auto id=ds.id;seq.samples.push_back(ds);DrumLane lane;lane.name=key;lane.sampleId=id;lane.steps.resize(16);pat.lanes.push_back(lane);}
        pat.lanes[0].steps[0].active=true;pat.lanes[1].steps[4].active=true;for(int i=0;i<16;i+=2)pat.lanes[2].steps[static_cast<std::size_t>(i)].active=true;auto pid=pat.id;seq.patterns.push_back(pat);PatternPlacement placement;placement.patternId=pid;placement.repeats=2;drumTrack.patternClips.push_back(placement);seq.tracks.push_back(drumTrack);
        AudioEngine seqEngine;seqEngine.publish(seq);auto seqAudio=seqEngine.renderOffline(MusicalTime::ticksToSamples(2*pat.lengthTicks(),90,48000));require(energy(seqAudio)>50.0,"step sequencer renders native drums");
        const auto seqPath=std::filesystem::temp_directory_path()/"flowdaw_seq.flow";ProjectSerializer::save(seq,seqPath);auto seqLoaded=ProjectSerializer::load(seqPath);require(seqLoaded.patterns.size()==1&&seqLoaded.patterns[0].lanes.size()==3,"sequencer project roundtrip");require(seqLoaded.patterns[0].lanes[1].steps[4].active,"sequencer step persists");require(seqLoaded.samples[0].audio&&seqLoaded.samples[0].nativeKey=="kick","native drum rehydrates");

        Project groove;groove.transport.bpm=90;Track gt;gt.name="Groove";SampleAsset hs;hs.name="hat";hs.nativeKey="hat";hs.audio=std::make_shared<AudioBuffer>(makeNativeDrum("hat",groove.sampleRate));auto hid=hs.id;groove.samples.push_back(hs);Pattern gp;gp.stepCount=16;gp.stepsPerBeat=4;DrumLane gl;gl.name="Hat";gl.sampleId=hid;gl.steps.resize(16);gl.steps[1].active=true;gl.steps[1].velocity=0.5f;gl.steps[1].probability=1.0f;gp.lanes.push_back(gl);auto gpid=gp.id;groove.patterns.push_back(gp);PatternPlacement gpl;gpl.patternId=gpid;gt.patternClips.push_back(gpl);groove.tracks.push_back(gt);
        AudioEngine ge;ge.publish(groove);auto straight=ge.renderOffline(30000);auto straightOnset=firstAudibleFrame(straight);require(straightOnset>0,"straight groove onset");
        groove.patterns[0].swing=1.0f;ge.publish(groove);auto swung=ge.renderOffline(30000);auto swungOnset=firstAudibleFrame(swung);require(swungOnset>straightOnset+3000,"swing delays off subdivision");
        groove.patterns[0].swing=0.0f;groove.patterns[0].lanes[0].steps[1].microTicks=120;ge.publish(groove);auto micro=ge.renderOffline(30000);require(firstAudibleFrame(micro)>straightOnset+3500,"microtiming moves hit");
        groove.patterns[0].lanes[0].steps[1].microTicks=0;groove.patterns[0].lanes[0].steps[1].probability=0.0f;ge.publish(groove);require(energy(ge.renderOffline(30000))==0.0,"zero probability suppresses hit");
        groove.patterns[0].lanes[0].steps[1].probability=1.0f;groove.patterns[0].humanize=0.8f;ge.publish(groove);auto humanA=ge.renderOffline(30000);ge.publish(groove);auto humanB=ge.renderOffline(30000);require(humanA.interleaved==humanB.interleaved,"humanize is deterministic per event");require(firstAudibleFrame(humanA)!=straightOnset,"humanize changes timing");
        groove.patterns[0].humanize=0.0f;groove.patterns[0].lanes[0].mute=true;ge.publish(groove);require(energy(ge.renderOffline(30000))==0.0,"lane mute suppresses audio");
        groove.patterns[0].lanes[0].mute=false;groove.patterns[0].swing=0.23f;groove.patterns[0].humanize=0.31f;const auto groovePath=std::filesystem::temp_directory_path()/"flowdaw_groove.flow";ProjectSerializer::save(groove,groovePath);auto grooveLoaded=ProjectSerializer::load(groovePath);require(std::abs(grooveLoaded.patterns[0].swing-0.23f)<0.001f&&std::abs(grooveLoaded.patterns[0].humanize-0.31f)<0.001f,"groove persists");require(std::abs(grooveLoaded.patterns[0].lanes[0].steps[1].velocity-0.5f)<0.001f,"velocity persists");

        std::cout<<"FLOWDAW core + sequencer/groove tests: PASS\n";
        std::cout<<"Rendered: "<<out<<"\n";
        return 0;
    }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
