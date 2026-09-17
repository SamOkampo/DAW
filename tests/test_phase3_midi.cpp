#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Midi.hpp"
#include "flowdaw/NativeInstruments.hpp"
#include "flowdaw/Serialization.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static double energy(const AudioBuffer&a,SampleIndex begin,SampleIndex end){begin=std::max<SampleIndex>(0,begin);end=std::min<SampleIndex>(a.frames(),end);double e=0;for(SampleIndex f=begin;f<end;++f)for(int c=0;c<a.channels;++c)e+=std::abs(a.interleaved[static_cast<std::size_t>(f*a.channels+c)]);return e;}
static double estimateFrequency(const AudioBuffer&a,SampleIndex start,SampleIndex length){int crossings=0;float prev=0;const auto end=std::min(a.frames(),start+length);for(SampleIndex f=start;f<end;++f){float x=a.interleaved[static_cast<std::size_t>(f*a.channels)];if(prev<=0&&x>0)++crossings;prev=x;}const double sec=static_cast<double>(end-start)/a.sampleRate;return sec>0?crossings/sec:0;}

int main(){
 try{
    require(std::abs(midiNoteFrequency(69)-440.0)<0.001,"A4 frequency");
    require(midiNoteName(60)=="C4"&&midiNoteName(61)=="C#4","MIDI note naming");
    require(snapMidiTick(370,240)==480&&snapMidiTick(100,240)==0,"MIDI grid snapping");
    require(pitchInScale(60,0,"minor")&&pitchInScale(63,0,"minor")&&!pitchInScale(64,0,"minor"),"C minor scale membership");
    require(pitchInScale(69,9,"minor_pentatonic")&&!pitchInScale(71,9,"minor_pentatonic"),"minor pentatonic membership");

    InstrumentState keys;keys.enabled=true;keys.type="flow_keys";keys.attackMs=2;keys.releaseMs=30;keys.tone=0.05f;
    auto note=renderNativeInstrumentNote(keys,69,1.0f,24000,48000,120.0);
    require(note.frames()>24000&&energy(note,1000,22000)>100.0,"native keys note audible");
    require(std::abs(estimateFrequency(note,5000,12000)-440.0)<10.0,"native keys approximately tune to MIDI pitch");

    auto dry=keys;dry.delayMix=0.0f;auto wet=keys;wet.delayMix=.45f;wet.delayTicks=kPPQ/2;
    auto dryAudio=renderNativeInstrumentNote(dry,60,1.0f,4800,48000,120.0);auto wetAudio=renderNativeInstrumentNote(wet,60,1.0f,4800,48000,120.0);
    require(wetAudio.frames()>dryAudio.frames()+15000,"tempo delay must extend note tail");
    require(energy(wetAudio,15000,30000)>1.0,"tempo delay must produce audible tail");

    Project p;p.name="Phase 3 MIDI";p.transport.bpm=120.0;
    Pattern midi;midi.name="Melody 1";midi.stepCount=32;midi.stepsPerBeat=4;midi.instrument=keys;midi.instrument.gain=.75f;midi.scaleRoot=0;midi.scaleType="minor";midi.midiGridTicks=kPPQ/4;midi.midiDefaultLengthTicks=kPPQ/2;
    MidiNote n;n.startTick=kPPQ;n.lengthTicks=kPPQ;n.pitch=69;n.velocity=.9f;midi.midiNotes.push_back(n);const Id patternId=midi.id;p.patterns.push_back(midi);
    Track instrument;instrument.name="INSTRUMENT";PatternPlacement pp;pp.patternId=patternId;instrument.patternClips.push_back(pp);p.tracks.push_back(instrument);
    AudioEngine engine;engine.publish(p);auto rendered=engine.renderOffline(80000);const SampleIndex beat=24000;
    require(energy(rendered,0,beat-1000)<0.001,"MIDI note must respect Arrangement start tick");
    require(energy(rendered,beat+1000,beat+16000)>50.0,"MIDI note must render through native instrument");

    const auto path=std::filesystem::temp_directory_path()/"flowdaw_phase3_v10.flow";ProjectSerializer::save(p,path);auto loaded=ProjectSerializer::load(path,false);std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");
    require(loaded.formatVersion==10,"project must save/load as v10");
    require(loaded.patterns.size()==1&&loaded.patterns[0].midiNotes.size()==1,"MIDI notes must persist");
    auto const&lp=loaded.patterns[0];require(lp.instrument.enabled&&lp.instrument.type=="flow_keys","native instrument must persist");
    require(std::abs(lp.instrument.gain-.75f)<.001f&&lp.scaleRoot==0&&lp.scaleType=="minor","instrument and scale controls must persist");
    require(lp.midiNotes[0].pitch==69&&lp.midiNotes[0].startTick==kPPQ&&lp.midiNotes[0].lengthTicks==kPPQ,"MIDI note timing/pitch must persist");

    const auto legacyPath=std::filesystem::temp_directory_path()/"flowdaw_phase3_legacy_v7.flow";
    {std::ofstream f(legacyPath);f<<"FLOWDAW_PROJECT 7\nNAME \"legacy\"\nSAMPLE_RATE 48000\nBPM 90\nPLAYHEAD 0\nMASTER 1 0\nSAMPLES 0\nTRACKS 0\nPATTERNS 0\nEND\n";}
    auto legacy=ProjectSerializer::load(legacyPath,false);std::filesystem::remove(legacyPath);require(legacy.formatVersion==10,"v7 projects must migrate to v10 in memory");

    std::cout<<"FLOWDAW Phase 3 MIDI/instrument tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
