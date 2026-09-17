#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/NativeDrums.hpp"
#include "flowdaw/SampleAnalysis.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/TimeStretch.hpp"
#include "flowdaw/Undo.hpp"
#include "flowdaw/Wav.hpp"
#include "flowdaw/Waveform.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static AudioBuffer tone(int sr=48000,double seconds=1.0,double hz=220.0){AudioBuffer a;a.sampleRate=sr;a.channels=1;auto n=static_cast<int>(sr*seconds);a.interleaved.resize(static_cast<std::size_t>(n));for(int i=0;i<n;++i)a.interleaved[static_cast<std::size_t>(i)]=0.25f*std::sin(2.0*3.141592653589793*hz*i/sr);return a;}
static AudioBuffer clickTrack(double bpm,int beats=24,int sr=48000){AudioBuffer a;a.sampleRate=sr;a.channels=1;const auto interval=static_cast<SampleIndex>(std::llround(sr*60.0/bpm));const auto n=interval*beats;a.interleaved.assign(static_cast<std::size_t>(n),0.0f);for(int b=0;b<beats;++b){const auto start=interval*b;for(int i=0;i<400&&start+i<n;++i)a.interleaved[static_cast<std::size_t>(start+i)]=static_cast<float>(std::exp(-i/55.0));}return a;}
static double energy(const AudioBuffer&a){double e=0;for(float x:a.interleaved)e+=std::abs(x);return e;}
static double estimateFrequency(const AudioBuffer&a,SampleIndex start,SampleIndex length){int crossings=0;float prev=0;for(SampleIndex f=start;f<std::min(a.frames(),start+length);++f){float x=a.interleaved[static_cast<std::size_t>(f*a.channels)];if(prev<=0&&x>0)++crossings;prev=x;}const double sec=static_cast<double>(std::min(length,a.frames()-start))/a.sampleRate;return sec>0?crossings/sec:0;}
static SampleIndex firstAudible(const AudioBuffer&a,float threshold=0.01f){for(SampleIndex f=0;f<a.frames();++f)if(std::abs(a.interleaved[static_cast<std::size_t>(f*a.channels)])>threshold)return f;return -1;}

static Project makeSequencer(int steps=16){
    Project seq;seq.name="Sequencer Test";seq.transport.bpm=90;Track drumTrack;drumTrack.name="Drums";Pattern pat;pat.name="Pattern 1";pat.stepCount=steps;pat.stepsPerBeat=4;
    for(auto const&key:{std::string("kick"),std::string("snare"),std::string("hat")}){SampleAsset ds;ds.name=key;ds.nativeKey=key;ds.audio=std::make_shared<AudioBuffer>(makeNativeDrum(key,seq.sampleRate));auto id=ds.id;seq.samples.push_back(ds);DrumLane lane;lane.name=key;lane.sampleId=id;lane.steps.resize(static_cast<std::size_t>(steps));pat.lanes.push_back(lane);}auto pid=pat.id;seq.patterns.push_back(pat);PatternPlacement placement;placement.patternId=pid;placement.repeats=1;drumTrack.patternClips.push_back(placement);seq.tracks.push_back(drumTrack);return seq;
}

int main(){
try{
    require(MusicalTime::toTicks({2,1,0})==3840,"bar to ticks");require(MusicalTime::fromTicks(4800).bar==2&&MusicalTime::fromTicks(4800).beat==2,"ticks to bar/beat");require(MusicalTime::ticksToSamples(3840,120,48000)==96000,"ticks to samples");require(MusicalTime::samplesToTicks(96000,120,48000)==3840,"samples to ticks");

    const auto tmp=std::filesystem::temp_directory_path()/"flowdaw_test.wav";auto a=tone();WavFile::writeFloat32(tmp,a);auto b=WavFile::read(tmp);require(b.sampleRate==48000&&b.channels==1&&b.frames()==a.frames(),"wav roundtrip");require(buildWaveform(b,100).size()==100,"waveform");

    Project p;p.name="Round Trip Beat";p.transport.bpm=90;SampleAsset s;s.name="tone";s.path=tmp;s.detectedBpm=91.25;s.bpmConfidence=.81f;s.audio=std::make_shared<AudioBuffer>(b);SampleSlice sl;sl.name="Slice 1";sl.startFrame=100;sl.endFrame=1000;sl.gain=.8f;sl.pan=.2f;sl.chokeGroup=2;s.slices.push_back(sl);auto sid=s.id;p.samples.push_back(s);Track tr;tr.name="Sample";Clip c;c.sampleId=sid;c.sourceLength=b.frames();c.lengthTicks=MusicalTime::samplesToTicks(b.frames(),90,48000);tr.clips.push_back(c);Effect fx;fx.type="gain";fx.value=.8f;tr.mixer.effects.push_back(fx);p.tracks.push_back(tr);
    const auto proj=std::filesystem::temp_directory_path()/"flowdaw_test.flow";ProjectSerializer::save(p,proj);auto loaded=ProjectSerializer::load(proj);require(loaded.formatVersion==7&&loaded.samples[0].slices.size()==1,"v7 slice project roundtrip");require(std::abs(loaded.samples[0].detectedBpm-91.25)<.01&&std::abs(loaded.samples[0].bpmConfidence-.81f)<.01f,"BPM metadata persists");require(std::abs(loaded.samples[0].slices[0].gain-.8f)<.001f&&loaded.samples[0].slices[0].chokeGroup==2,"pad controls persist");require(loaded.tracks[0].mixer.effects.size()==1,"effects persist");

    UndoStack undo;auto before=p;p.tracks[0].clips[0].startTick=960;auto after=p;undo.commit(before,after,"Move clip");require(undo.undo(p)&&p.tracks[0].clips[0].startTick==0,"undo");require(undo.redo(p)&&p.tracks[0].clips[0].startTick==960,"redo");
    p=before;AudioEngine engine;engine.publish(p);require(energy(engine.renderOffline(48000))>10.0,"offline engine renders audible signal");

    for(auto const&key:{"kick","snare","hat","openhat","clap","rim","perc"}){auto drum=makeNativeDrum(key,48000);require(drum.frames()>1000&&energy(drum)>1.0,"native drum library");}

    auto seq=makeSequencer(64);auto&pat=seq.patterns[0];pat.lanes[0].steps[0].active=true;pat.lanes[0].steps[63].active=true;pat.lanes[1].steps[4].active=true;for(int i=0;i<64;i+=2)pat.lanes[2].steps[static_cast<std::size_t>(i)].active=true;AudioEngine seqEngine;seqEngine.publish(seq);require(energy(seqEngine.renderOffline(MusicalTime::ticksToSamples(pat.lengthTicks(),90,48000)))>50.0,"64-step sequencer renders");const auto seqPath=std::filesystem::temp_directory_path()/"flowdaw_seq.flow";ProjectSerializer::save(seq,seqPath);auto seqLoaded=ProjectSerializer::load(seqPath);require(seqLoaded.patterns[0].stepCount==64&&seqLoaded.patterns[0].lanes[0].steps[63].active,"step 64 persists");

    auto straight=makeSequencer();straight.patterns[0].lanes[0].steps[1].active=true;AudioEngine straightEngine;straightEngine.publish(straight);auto straightOnset=firstAudible(straightEngine.renderOffline(30000));auto swung=straight;swung.patterns[0].swing=1.0f;AudioEngine swingEngine;swingEngine.publish(swung);require(firstAudible(swingEngine.renderOffline(30000))>straightOnset+3000,"swing affects scheduling");swung.patterns[0].lanes[0].steps[1].probability=0.0f;swingEngine.publish(swung);require(energy(swingEngine.renderOffline(30000))<0.01,"probability zero suppresses hit");swung.patterns[0].lanes[0].steps[1].probability=1.0f;swung.patterns[0].lanes[0].mute=true;swingEngine.publish(swung);require(energy(swingEngine.renderOffline(30000))<0.01,"lane mute suppresses hit");

    auto clicks90=clickTrack(90);auto bpm90=detectBpm(clicks90,60,180);require(bpm90.valid()&&std::abs(bpm90.bpm-90)<2.0,"BPM detection 90");require(detectTransients(clicks90,.9,.08).size()>=20,"transient detection");auto equal=makeEqualSlices(clicks90,8);require(equal.size()==8&&equal.front().startFrame==0&&equal.back().endFrame==clicks90.frames(),"equal slices cover source");
    auto sinus=tone(48000,2.0,220);auto shortTone=timeStretchWsola(sinus,.8);require(std::llabs(shortTone.frames()-static_cast<SampleIndex>(sinus.frames()*.8))<=1,"WSOLA duration");require(std::abs(estimateFrequency(shortTone,5000,30000)-220)<8,"WSOLA pitch");auto clicks82=clickTrack(82,28);auto matched90=matchBpmWsola(clicks82,82,90);auto matchedEstimate=detectBpm(matched90,70,120);require(matchedEstimate.valid()&&std::abs(matchedEstimate.bpm-90)<3.0,"82 BPM matches to 90 BPM");

    const auto srcPath=std::filesystem::temp_directory_path()/"flowdaw_82.wav";WavFile::writeFloat32(srcPath,clicks82);Project smart;smart.transport.bpm=90;SampleAsset original;original.name="82 source";original.path=srcPath;original.audio=std::make_shared<AudioBuffer>(clicks82);original.detectedBpm=82;auto sourceId=original.id;smart.samples.push_back(original);SampleAsset derived;derived.name="matched";derived.sourceSampleId=sourceId;derived.timeRatio=82.0/90.0;derived.detectedBpm=90;derived.audio=std::make_shared<AudioBuffer>(matched90);auto derivedId=derived.id;smart.samples.push_back(derived);Track st;Clip sc;sc.sampleId=derivedId;sc.sourceLength=matched90.frames();st.clips.push_back(sc);smart.tracks.push_back(st);const auto smartPath=std::filesystem::temp_directory_path()/"flowdaw_smart.flow";ProjectSerializer::save(smart,smartPath);auto smartLoaded=ProjectSerializer::load(smartPath);require(smartLoaded.samples[0].audio&&smartLoaded.samples[0].audio->frames()==clicks82.frames(),"original source unchanged after reopen");require(smartLoaded.samples[1].audio&&std::llabs(smartLoaded.samples[1].audio->frames()-matched90.frames())<=1,"derived asset regenerated");

    auto segmented=std::make_shared<AudioBuffer>();segmented->sampleRate=48000;segmented->channels=1;segmented->interleaved.assign(400,0.0f);std::fill(segmented->interleaved.begin()+100,segmented->interleaved.begin()+200,0.8f);std::fill(segmented->interleaved.begin()+200,segmented->interleaved.begin()+300,-0.5f);AudioEngine preview;require(preview.triggerPreview(segmented,100,100),"queue slice preview");auto previewBlock=preview.renderDeviceBlockForTest(160);require(energy(previewBlock)>40.0,"slice preview audible while transport stopped");double tail=0;for(SampleIndex f=110;f<160;++f)tail+=std::abs(previewBlock.interleaved[static_cast<std::size_t>(f*2)]);require(tail<0.001,"preview stops at slice boundary");require(preview.triggerPreview(segmented,200,100),"queue second slice");require(preview.renderDeviceBlockForTest(80).interleaved[0]<-0.2f,"second pad starts from own slice");

    Project chops;chops.transport.bpm=90;SampleAsset chopSample;chopSample.name="Chop source";chopSample.audio=segmented;SampleSlice sliceA;sliceA.name="A";sliceA.startFrame=100;sliceA.endFrame=200;SampleSlice sliceB;sliceB.name="B";sliceB.startFrame=200;sliceB.endFrame=300;const auto sliceAId=sliceA.id,sliceBId=sliceB.id;chopSample.slices={sliceA,sliceB};const auto chopSampleId=chopSample.id;chops.samples.push_back(chopSample);Pattern chopPattern;chopPattern.name="Recorded Chops";ChopEvent hitA;hitA.recordedTick=0;hitA.tick=0;hitA.sampleId=chopSampleId;hitA.sliceId=sliceAId;ChopEvent hitB;hitB.recordedTick=kPPQ;hitB.tick=kPPQ;hitB.sampleId=chopSampleId;hitB.sliceId=sliceBId;chopPattern.chopEvents={hitA,hitB};const auto chopPatternId=chopPattern.id;chops.patterns.push_back(chopPattern);Track chopTrack;chopTrack.name="CHOPS";PatternPlacement chopPlacement;chopPlacement.patternId=chopPatternId;chopTrack.patternClips.push_back(chopPlacement);chops.tracks.push_back(chopTrack);AudioEngine chopEngine;chopEngine.publish(chops);const auto beatSamples=MusicalTime::ticksToSamples(kPPQ,90,48000);auto chopRender=chopEngine.renderOffline(beatSamples+600);require(chopRender.interleaved[0]>0.2f,"first recorded chop resolves slice");require(chopRender.interleaved[static_cast<std::size_t>(beatSamples*2)]<-0.1f,"second recorded chop resolves slice");double gapEnergy=0;for(SampleIndex f=1000;f<beatSamples-1000;++f)gapEnergy+=std::abs(chopRender.interleaved[static_cast<std::size_t>(f*2)]);require(gapEnergy<0.001,"recorded chops stop at slice boundaries");const auto chopPath=std::filesystem::temp_directory_path()/"flowdaw_chops_v7.flow";ProjectSerializer::save(chops,chopPath);auto chopsLoaded=ProjectSerializer::load(chopPath,false);require(chopsLoaded.formatVersion==7&&chopsLoaded.patterns[0].chopEvents.size()==2,"v7 ChopEvents persist");require(chopsLoaded.patterns[0].chopEvents[1].recordedTick==kPPQ&&chopsLoaded.patterns[0].chopEvents[1].sliceId==sliceBId,"recorded timing and slice identity persist");

    std::filesystem::remove(tmp);std::filesystem::remove(proj);std::filesystem::remove(seqPath);std::filesystem::remove(srcPath);std::filesystem::remove(smartPath);std::filesystem::remove(chopPath);
    std::cout<<"FLOWDAW Phase 0/1/2 v7 integration tests: PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
