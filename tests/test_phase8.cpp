#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/PluginHost.hpp"
#include "flowdaw/RealtimePluginGraph.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace flowdaw;

static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static bool near(float a,float b,float eps=0.001f){return std::abs(a-b)<=eps;}

class FakeLatencyProcessor final:public IPluginProcessor{
public:
    FakeLatencyProcessor(int latency,float gain):latency_(latency),gain_(gain){}
    bool prepare(int,int channels,std::string&)override{channels_=channels;line_.assign(static_cast<std::size_t>(std::max(0,latency_))*static_cast<std::size_t>(channels_),0.0f);write_=0;return true;}
    void setState(const std::string&)override{}
    std::string state()const override{return"fake-state";}
    void process(AudioBuffer&buffer)override{(void)processRealtime(buffer.interleaved.data(),buffer.frames(),buffer.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    int latencySamples()const noexcept override{return latency_;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_)return false;
        if(latency_<=0){for(SampleIndex f=0;f<frames;++f)for(int c=0;c<channels_;++c)data[static_cast<std::size_t>(f*channels_+c)]*=gain_;return true;}
        for(SampleIndex f=0;f<frames;++f){const auto ringBase=static_cast<std::size_t>(write_)*static_cast<std::size_t>(channels_);const auto ioBase=static_cast<std::size_t>(f)*static_cast<std::size_t>(channels_);for(int c=0;c<channels_;++c){const auto cc=static_cast<std::size_t>(c);const float in=data[ioBase+cc];data[ioBase+cc]=line_[ringBase+cc]*gain_;line_[ringBase+cc]=in;}if(++write_>=latency_)write_=0;}return true;
    }
    void resetRealtime()noexcept override{std::fill(line_.begin(),line_.end(),0.0f);write_=0;}
private:int latency_=0;float gain_=1.0f;int channels_=2,write_=0;std::vector<float>line_;
};

class FakeBackend final:public IExternalPluginBackend{
public:
    bool supports(const std::string&format)const override{return format=="vst3";}
    std::unique_ptr<IPluginProcessor> create(const PluginInstance&plugin,std::string&)override{
        const int latency=plugin.identifier.find("latency3")!=std::string::npos?3:(plugin.identifier.find("latency2")!=std::string::npos?2:0);
        return std::make_unique<FakeLatencyProcessor>(latency,0.5f);
    }
};

class BlockingProcessor final:public IPluginProcessor{
public:
    inline static std::atomic<bool> entered{false};
    inline static std::atomic<bool> release{false};
    bool prepare(int,int,std::string&)override{return true;}
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&buffer)override{(void)processRealtime(buffer.interleaved.data(),buffer.frames(),buffer.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*,SampleIndex,int)noexcept override{
        entered.store(true,std::memory_order_release);
        while(!release.load(std::memory_order_acquire))std::this_thread::yield();
        return true;
    }
};

class BlockingBackend final:public IExternalPluginBackend{
public:
    bool supports(const std::string&format)const override{return format=="vst3";}
    std::unique_ptr<IPluginProcessor> create(const PluginInstance&,std::string&)override{return std::make_unique<BlockingProcessor>();}
};

static PluginInstance fakePlugin(const std::string&id,float wet=1.0f){PluginInstance p;p.format="vst3";p.identifier=id;p.name="Fake Phase 8";p.enabled=true;p.bypass=false;p.wet=wet;return p;}

static void testDelayLine(){
    RealtimeDelayLine delay;delay.prepare(2,2);float source[]={1,1,2,2,3,3,4,4};float dest[8]{};delay.addDelayed(source,dest,4);
    require(near(dest[0],0)&&near(dest[2],0),"PDC delay emitted audio too early");require(near(dest[4],1)&&near(dest[6],2),"PDC delay did not preserve exact sample offset");
}

static void testPreparedChain(){
    PluginHost host;host.registerBackend(std::make_shared<FakeBackend>());RealtimePluginChain chain;std::vector<RealtimePluginIssue>issues;
    require(chain.prepare({fakePlugin("fake.latency3")},&host,48000,2,4,&issues),"realtime chain failed to prepare fake backend");require(issues.empty(),"unexpected realtime chain issue");require(chain.latencySamples()==3,"chain latency did not include processor latency");
    float audio[16];std::fill(std::begin(audio),std::end(audio),1.0f);chain.process(audio,8);for(int f=0;f<3;++f)require(near(audio[f*2],0),"latency processor emitted before reported latency");for(int f=3;f<8;++f)require(near(audio[f*2],0.5f),"realtime processor gain/output mismatch");

    RealtimePluginChain wetChain;issues.clear();require(wetChain.prepare({fakePlugin("fake.latency2",0.5f)},&host,48000,2,8,&issues),"wet/dry chain failed to prepare");float wetAudio[12];std::fill(std::begin(wetAudio),std::end(wetAudio),1.0f);wetChain.process(wetAudio,6);require(near(wetAudio[0],0)&&near(wetAudio[2],0),"wet/dry dry path was not latency aligned");for(int f=2;f<6;++f)require(near(wetAudio[f*2],0.75f),"latency-aligned wet/dry mix is incorrect");
}

static void testRealtimeMasterGraph(){
    auto host=std::make_shared<PluginHost>();host->registerBackend(std::make_shared<FakeBackend>());AudioEngine engine;engine.configureExternalDevice(48000,4,true);engine.setPluginHost(host);engine.setInputMonitoring(true);
    Project project;project.master.plugins.push_back(fakePlugin("fake.latency3"));engine.publish(project);
    AudioBuffer input;input.sampleRate=48000;input.channels=1;input.interleaved.assign(8,1.0f);auto output=engine.processInputBlockForTest(input);require(output.frames()==8,"engine test block size changed");
    for(int f=0;f<3;++f)require(near(output.interleaved[static_cast<std::size_t>(f*2)],0),"master plugin latency was not present in realtime AudioEngine path");
    for(int f=3;f<8;++f)require(near(output.interleaved[static_cast<std::size_t>(f*2)],0.35355f,0.002f),"external master plugin did not process monitored audio in realtime engine");
}

static void testBuiltinRealtimeChain(){
    auto gain=makeBuiltinPlugin("flow.gain");setPluginParameter(gain,"gain",0.25f);RealtimePluginChain chain;std::vector<RealtimePluginIssue>issues;require(chain.prepare({gain},nullptr,48000,2,8,&issues),"builtin chain should not require external host");float audio[]={1,1,-1,-1};chain.process(audio,2);require(near(audio[0],0.25f)&&near(audio[2],-0.25f),"builtin realtime chain processing changed");
}

static void testSafeGraphReclamation(){
    BlockingProcessor::entered.store(false,std::memory_order_relaxed);BlockingProcessor::release.store(false,std::memory_order_relaxed);
    auto host=std::make_shared<PluginHost>();host->registerBackend(std::make_shared<BlockingBackend>());
    AudioEngine engine;engine.configureExternalDevice(48000,64,true);engine.setPluginHost(host);engine.setInputMonitoring(true);

    PluginInstance blocking;blocking.format="vst3";blocking.identifier="test.blocking";blocking.name="Blocking test processor";
    Project first;first.master.plugins.push_back(blocking);engine.publish(first);
    AudioBuffer input;input.sampleRate=48000;input.channels=1;input.interleaved.assign(64,1.0f);
    std::thread audioThread([&]{auto output=engine.processInputBlockForTest(input);(void)output;});

    bool didEnter=false;for(int i=0;i<200000;++i){if(BlockingProcessor::entered.load(std::memory_order_acquire)){didEnter=true;break;}std::this_thread::yield();}
    if(!didEnter){BlockingProcessor::release.store(true,std::memory_order_release);audioThread.join();require(false,"blocking processor never entered realtime callback");}

    Project replacement;engine.publish(replacement);
    require(engine.collectRetiredGraphs()==1,"active callback graph was reclaimed before reader exited");
    BlockingProcessor::release.store(true,std::memory_order_release);audioThread.join();
    require(engine.collectRetiredGraphs()==0,"retired graph was not reclaimed on control thread after reader exited");

    for(int i=0;i<8;++i){Project p;p.master.volume=1.0f-static_cast<float>(i)*0.01f;engine.publish(p);}
    require(engine.collectRetiredGraphs()==0,"idle repeated graph publication accumulated retired graphs");
}

static void testTrackBusRoutingPdc(){
    auto host=std::make_shared<PluginHost>();host->registerBackend(std::make_shared<FakeBackend>());
    AudioEngine engine;engine.configureExternalDevice(48000,16,false);engine.setPluginHost(host);

    Project project;
    SampleAsset sample;sample.audio=std::make_shared<AudioBuffer>();sample.audio->sampleRate=48000;sample.audio->channels=1;sample.audio->interleaved.assign(32,0.0f);sample.audio->interleaved[0]=0.4f;const Id sampleId=sample.id;project.samples.push_back(sample);

    Bus bus;bus.name="PDC Bus";bus.mixer.plugins.push_back(fakePlugin("fake.latency2"));const Id busId=bus.id;project.buses.push_back(bus);

    Track direct;direct.name="Direct";Clip directClip;directClip.sampleId=sampleId;directClip.sourceLength=32;direct.clips.push_back(directClip);MixerSend send;send.busId=busId;send.gain=0.5f;direct.sends.push_back(send);
    Track throughBus;throughBus.name="Through Bus";throughBus.outputBusId=busId;throughBus.mixer.plugins.push_back(fakePlugin("fake.latency3"));Clip busClip;busClip.sampleId=sampleId;busClip.sourceLength=32;throughBus.clips.push_back(busClip);
    project.tracks.push_back(direct);project.tracks.push_back(throughBus);

    engine.publish(project);engine.play();auto output=engine.renderDeviceBlockForTest(16);
    for(int frame=0;frame<5;++frame)require(near(output.interleaved[static_cast<std::size_t>(frame*2)],0.0f,0.002f),"PDC route emitted impulse before longest path latency");
    const float expected=0.4f*std::sqrt(0.5f)*1.5f;
    require(near(output.interleaved[10],expected,0.004f),"direct/send/bus paths did not align on the same compensated sample");
    require(near(output.interleaved[12],0.0f,0.004f),"PDC impulse smeared into the next sample");
}


static void testOfflineExportParity(){
    auto host=std::make_shared<PluginHost>();host->registerBackend(std::make_shared<FakeBackend>());
    Project project;project.transport.bpm=120.0;

    SampleAsset sample;sample.audio=std::make_shared<AudioBuffer>();sample.audio->sampleRate=48000;sample.audio->channels=1;sample.audio->interleaved.assign(128,0.0f);sample.audio->interleaved[0]=0.4f;const Id sampleId=sample.id;project.samples.push_back(sample);

    Bus bus;bus.name="Offline parity bus";bus.mixer.plugins.push_back(fakePlugin("fake.latency2"));const Id busId=bus.id;project.buses.push_back(bus);
    Track track;track.name="Offline parity track";track.outputBusId=busId;track.mixer.plugins.push_back(fakePlugin("fake.latency3"));
    Clip clip;clip.sampleId=sampleId;clip.sourceLength=128;clip.lengthTicks=kPPQ;track.clips.push_back(clip);project.tracks.push_back(track);
    project.master.plugins.push_back(fakePlugin("fake.latency2"));

    AudioEngine realtime;realtime.configureExternalDevice(48000,16,false);realtime.setPluginHost(host);realtime.publish(project);realtime.play();
    auto device=realtime.renderDeviceBlockForTest(32);

    AudioEngine offline;offline.configureExternalDevice(48000,16,false);offline.setPluginHost(host);offline.publish(project);
    auto directBounce=offline.renderOffline(32);
    require(device.interleaved.size()==directBounce.interleaved.size(),"offline parity block length mismatch");
    for(std::size_t i=0;i<device.interleaved.size();++i)require(near(device.interleaved[i],directBounce.interleaved[i],0.0001f),"offline AudioEngine graph diverged from realtime graph");

    auto exported=renderProjectOffline(project,0.0,host);
    require(exported.frames()>=32,"project export ended before test graph output");
    for(std::size_t i=0;i<device.interleaved.size();++i)require(near(device.interleaved[i],exported.interleaved[i],0.0001f),"project export with external host diverged from realtime graph");
}


static void testRealtimeMeterPrimitive(){
    RealtimeMeterState meter;
    const float audio[]={-1.0f,0.5f,0.5f,-0.25f};
    meter.process(audio,2,2);
    const auto reading=meter.snapshot();
    require(near(reading.samplePeakLeft,1.0f),"sample-peak left meter mismatch");
    require(near(reading.samplePeakRight,0.5f),"sample-peak right meter mismatch");
    require(near(reading.rmsLeft,std::sqrt(0.625f),0.0001f),"RMS left meter mismatch");
    require(near(reading.rmsRight,std::sqrt(0.15625f),0.0001f),"RMS right meter mismatch");
    meter.reset();
    const auto reset=meter.snapshot();
    require(near(reset.samplePeakLeft,0.0f)&&near(reset.truePeakLeft,0.0f)&&near(reset.rmsRight,0.0f),"meter reset did not publish silence");
}

static void testRealtimeTruePeakEstimate(){
    RealtimeMeterState meter;
    const float firstBlock[]={-1.0f,1.0f,1.0f};
    meter.process(firstBlock,3,1);
    const float secondBlock[]={-1.0f,0.0f};
    meter.process(secondBlock,2,1);
    const auto reading=meter.snapshot();
    require(near(reading.samplePeakLeft,1.0f),"true-peak fixture sample peak changed");
    require(reading.truePeakLeft>1.24f&&reading.truePeakLeft<1.26f,"4x inter-sample true-peak estimate did not detect overshoot");
    require(near(reading.truePeakRight,reading.truePeakLeft,0.0001f),"mono true-peak channels diverged");
}

static void testAudioEngineMeters(){
    AudioEngine engine;engine.configureExternalDevice(48000,32,false);
    Project project;
    SampleAsset sample;sample.audio=std::make_shared<AudioBuffer>();sample.audio->sampleRate=48000;sample.audio->channels=1;sample.audio->interleaved.assign(64,0.25f);const Id sampleId=sample.id;project.samples.push_back(sample);
    Bus bus;bus.name="Meter Bus";const Id busId=bus.id;project.buses.push_back(bus);
    Track track;track.name="Meter Track";track.outputBusId=busId;const Id trackId=track.id;Clip clip;clip.sampleId=sampleId;clip.sourceLength=64;clip.lengthTicks=kPPQ;track.clips.push_back(clip);project.tracks.push_back(track);

    engine.publish(project);engine.play();(void)engine.renderDeviceBlockForTest(32);
    const auto snapshot=engine.meterSnapshot();
    require(snapshot.tracks.size()==1&&snapshot.tracks[0].id==trackId,"track meter identity/snapshot mismatch");
    require(snapshot.buses.size()==1&&snapshot.buses[0].id==busId,"bus meter identity/snapshot mismatch");
    require(snapshot.tracks[0].level.samplePeakLeft>0.0f&&snapshot.tracks[0].level.rmsLeft>0.0f,"track meter did not observe routed audio");
    require(snapshot.buses[0].level.samplePeakLeft>0.0f&&snapshot.buses[0].level.rmsLeft>0.0f,"bus meter did not observe routed audio");
    require(snapshot.master.samplePeakLeft>0.0f&&snapshot.master.rmsLeft>0.0f,"master meter did not observe post-graph audio");

    Project replacement;engine.publish(replacement);
    const auto reset=engine.meterSnapshot();
    require(reset.tracks.empty()&&reset.buses.empty(),"meter bank retained removed routes after graph publication");
    require(near(reset.master.samplePeakLeft,0.0f)&&near(reset.master.rmsLeft,0.0f),"new master meter did not start at silence");
}

int main(){try{testDelayLine();testPreparedChain();testRealtimeMasterGraph();testBuiltinRealtimeChain();testSafeGraphReclamation();testTrackBusRoutingPdc();testOfflineExportParity();testRealtimeMeterPrimitive();testRealtimeTruePeakEstimate();testAudioEngineMeters();std::cout<<"Phase 8 realtime plugin graph foundation OK\n";return 0;}catch(const std::exception&e){std::cerr<<"Phase 8 test failed: "<<e.what()<<"\n";return 1;}}
