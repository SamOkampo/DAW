#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Automation.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Wav.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static double energy(const AudioBuffer&a,SampleIndex begin,SampleIndex end){begin=std::max<SampleIndex>(0,begin);end=std::min<SampleIndex>(a.frames(),end);double e=0;for(SampleIndex f=begin;f<end;++f)for(int c=0;c<a.channels;++c)e+=std::abs(a.interleaved[static_cast<std::size_t>(f*a.channels+c)]);return e;}
static std::shared_ptr<AudioBuffer> constantAudio(float value=0.1f,SampleIndex frames=48000){auto a=std::make_shared<AudioBuffer>();a->sampleRate=48000;a->channels=1;a->interleaved.assign(static_cast<std::size_t>(frames),value);return a;}

int main(){
 try{
    AutomationLane curve;curve.points={{0,0.0f},{kPPQ,1.0f},{kPPQ*2,.5f}};normalizeAutomationLane(curve);require(std::abs(automationValueAt(curve,kPPQ/2,1)-.5f)<.001f,"automation interpolation");require(std::abs(automationValueAt(curve,kPPQ*3,1)-.5f)<.001f,"automation holds last value");

    AudioEngine recorder;AudioBuffer input;input.sampleRate=48000;input.channels=1;input.interleaved.resize(256);for(int i=0;i<256;++i)input.interleaved[static_cast<std::size_t>(i)]=static_cast<float>(i)/255.0f;require(recorder.beginRecording(1024),"begin recording");recorder.setInputMonitoring(true);auto monitored=recorder.processInputBlockForTest(input);recorder.processInputBlockForTest(input);auto captured=recorder.finishRecording();require(captured.channels==1&&captured.frames()==512,"recording captures bounded input frames");require(std::abs(captured.interleaved[255]-1.0f)<.001f,"recording preserves input samples");require(energy(monitored,0,256)>10.0,"input monitoring is audible");

    Project p;p.name="Phase 4";p.transport.bpm=120;SampleAsset src;src.name="Voice";src.audio=constantAudio();const Id sid=src.id;p.samples.push_back(src);Track vocal;vocal.name="VOCAL";vocal.armed=true;vocal.inputMonitor=true;RecordingTake take;take.name="Take 1";take.sampleId=sid;take.startTick=0;take.lengthTicks=kPPQ*2;vocal.takes.push_back(take);vocal.activeTakeId=take.id;const Id trackId=vocal.id;p.tracks.push_back(vocal);
    AudioEngine takeEngine;takeEngine.publish(p);auto takeRender=takeEngine.renderOffline(24000);require(energy(takeRender,0,20000)>100.0,"active recording take renders");

    AutomationLane vol;vol.target="track.volume";vol.targetId=trackId;vol.points={{0,0.05f},{kPPQ,1.0f}};p.automation.push_back(vol);takeEngine.publish(p);auto automated=takeEngine.renderOffline(24000);require(energy(automated,18000,23000)>energy(automated,1000,6000)*4.0,"track volume automation changes rendered level");

    Bus bus;bus.name="Vocal Bus";bus.mixer.volume=.5f;const Id busId=bus.id;p.buses.push_back(bus);p.tracks[0].outputBusId=busId;p.automation.clear();takeEngine.publish(p);auto busRender=takeEngine.renderOffline(24000);p.tracks[0].outputBusId=0;takeEngine.publish(p);auto directRender=takeEngine.renderOffline(24000);require(energy(busRender,1000,10000)<energy(directRender,1000,10000)*.6,"bus fader affects routed track");

    p.tracks[0].outputBusId=0;MixerSend send;send.busId=busId;send.gain=.5f;p.tracks[0].sends.push_back(send);takeEngine.publish(p);auto sendRender=takeEngine.renderOffline(24000);require(energy(sendRender,1000,10000)>energy(directRender,1000,10000)*1.1,"parallel send adds routed signal");

    AutomationLane sendCurve;sendCurve.target="send.gain";sendCurve.targetId=trackId;sendCurve.subTargetId=send.id;sendCurve.points={{0,0.0f},{kPPQ,1.0f}};p.automation.push_back(sendCurve);takeEngine.publish(p);auto sendAuto=takeEngine.renderOffline(24000);require(energy(sendAuto,18000,23000)>energy(sendAuto,1000,6000)*1.15,"send automation changes wet contribution");

    AudioEngine noInput;noInput.configureExternalDevice(48000,256,false);require(!noInput.inputAvailable(),"disconnected input state must remain unavailable before record UI arms capture");
    const auto projectPath=std::filesystem::temp_directory_path()/"flowdaw_phase4_v10.flow";ProjectSerializer::save(p,projectPath);auto loaded=ProjectSerializer::load(projectPath,false);require(loaded.formatVersion==11,"project must save/load as v11");require(loaded.buses.size()==1&&loaded.tracks[0].sends.size()==1,"buses and sends persist");require(loaded.tracks[0].takes.size()==1&&loaded.tracks[0].activeTakeId==take.id,"recording takes and comp selection persist");require(loaded.automation.size()==1&&loaded.automation[0].target=="send.gain","automation lanes persist");std::filesystem::remove(projectPath);std::filesystem::remove(projectPath.string()+".bak");

    const auto legacyPath=std::filesystem::temp_directory_path()/"flowdaw_phase4_legacy_v8.flow";{std::ofstream f(legacyPath);f<<"FLOWDAW_PROJECT 8\nNAME \"legacy\"\nSAMPLE_RATE 48000\nBPM 90\nPLAYHEAD 0\nMASTER 1 0\nSAMPLES 0\nTRACKS 0\nPATTERNS 0\nEND\n";}auto legacy=ProjectSerializer::load(legacyPath,false);std::filesystem::remove(legacyPath);require(legacy.formatVersion==11,"v8 projects migrate to v11");

    Project ex;ex.transport.bpm=120;SampleAsset es;es.name="tone";es.audio=constantAudio(.05f,24000);const Id esid=es.id;ex.samples.push_back(es);Track et;et.name="Beat One";Clip ec;ec.sampleId=esid;ec.sourceLength=24000;ec.lengthTicks=kPPQ;et.clips.push_back(ec);ex.tracks.push_back(et);require(projectEndTick(ex)==kPPQ,"export length follows Arrangement");const auto exportDir=std::filesystem::temp_directory_path()/"flowdaw_phase4_export";const auto mixPath=exportDir/"mix.wav";exportProjectWav(ex,mixPath,0);require(std::filesystem::exists(mixPath)&&WavFile::read(mixPath).frames()>=23999,"master WAV export");auto stems=exportTrackStems(ex,exportDir/"stems",0);require(stems.size()==1&&std::filesystem::exists(stems[0]),"track stem export");std::filesystem::remove_all(exportDir);
    const auto blockedParent=std::filesystem::temp_directory_path()/"flowdaw_export_blocked_parent";std::filesystem::remove_all(blockedParent);{std::ofstream f(blockedParent);f<<"not a directory";}bool invalidExportFailed=false;try{exportProjectWav(ex,blockedParent/"mix.wav",0);}catch(const std::exception&){invalidExportFailed=true;}require(invalidExportFailed,"export to an invalid destination must fail instead of reporting success");std::filesystem::remove(blockedParent);

    std::cout<<"FLOWDAW Phase 4 recording/automation/mixer tests: PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
