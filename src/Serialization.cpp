#include "flowdaw/Serialization.hpp"
#include "flowdaw/NativeDrums.hpp"
#include "flowdaw/TimeStretch.hpp"
#include "flowdaw/Wav.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace flowdaw {
namespace {
std::string q(const std::string&s){std::ostringstream o;o<<std::quoted(s);return o.str();}
void saveEffect(std::ofstream&f,const char*tag,const Effect&e){f<<tag<<" "<<e.id<<" "<<q(e.type)<<" "<<e.enabled<<" "<<e.value<<"\n";}
Effect loadEffect(std::ifstream&f,std::string&tag,const char*expected){Effect e;f>>tag;if(tag!=expected)throw std::runtime_error(std::string("Expected ")+expected);f>>e.id>>std::quoted(e.type)>>e.enabled>>e.value;return e;}
}

void ProjectSerializer::save(const Project&p,const std::filesystem::path&path){
    std::ofstream f(path);if(!f)throw std::runtime_error("Cannot save project");
    f<<"FLOWDAW_PROJECT 7\n";
    f<<"NAME "<<q(p.name)<<"\nSAMPLE_RATE "<<p.sampleRate<<"\nBPM "<<std::setprecision(12)<<p.transport.bpm<<"\nPLAYHEAD "<<p.transport.playheadTick<<"\n";
    f<<"MASTER "<<p.master.volume<<" "<<p.master.effects.size()<<"\n";for(auto const&e:p.master.effects)saveEffect(f,"MASTER_EFFECT",e);
    f<<"SAMPLES "<<p.samples.size()<<"\n";
    for(auto const&s:p.samples){
        f<<"SAMPLE "<<s.id<<" "<<q(s.name)<<" "<<q(s.path.generic_string())<<" "<<q(s.nativeKey)<<" "<<s.detectedBpm<<" "<<s.bpmConfidence<<" "<<s.sourceSampleId<<" "<<s.timeRatio<<" "<<s.slices.size()<<"\n";
        for(auto const&sl:s.slices)f<<"SLICE "<<sl.id<<" "<<q(sl.name)<<" "<<sl.startFrame<<" "<<sl.endFrame<<" "<<sl.gain<<" "<<sl.pan<<" "<<sl.chokeGroup<<"\n";
    }
    f<<"TRACKS "<<p.tracks.size()<<"\n";
    for(auto const&t:p.tracks){
        f<<"TRACK "<<t.id<<" "<<q(t.name)<<" "<<t.mixer.volume<<" "<<t.mixer.pan<<" "<<t.mixer.mute<<" "<<t.mixer.solo<<" "<<t.clips.size()<<" "<<t.patternClips.size()<<" "<<t.mixer.effects.size()<<"\n";
        for(auto const&c:t.clips)f<<"CLIP "<<c.id<<" "<<c.sampleId<<" "<<c.startTick<<" "<<c.lengthTicks<<" "<<c.sourceStart<<" "<<c.sourceLength<<" "<<c.gain<<" "<<c.loop<<"\n";
        for(auto const&pc:t.patternClips)f<<"PATCLIP "<<pc.id<<" "<<pc.patternId<<" "<<pc.startTick<<" "<<pc.repeats<<"\n";
        for(auto const&e:t.mixer.effects)saveEffect(f,"TRACK_EFFECT",e);
    }
    f<<"PATTERNS "<<p.patterns.size()<<"\n";
    for(auto const&pat:p.patterns){
        f<<"PATTERN "<<pat.id<<" "<<q(pat.name)<<" "<<pat.stepCount<<" "<<pat.stepsPerBeat<<" "<<pat.swing<<" "<<pat.humanize<<" "<<pat.lanes.size()<<" "<<pat.chopEvents.size()<<" "<<pat.chopQuantizeGridTicks<<" "<<pat.chopQuantizeStrength<<" "<<pat.chopHumanize<<"\n";
        for(auto const&lane:pat.lanes){
            f<<"LANE "<<q(lane.name)<<" "<<lane.sampleId<<" "<<lane.volume<<" "<<lane.pan<<" "<<lane.mute<<" "<<lane.solo<<" "<<lane.steps.size()<<"\n";
            for(auto const&st:lane.steps)f<<"STEP "<<st.active<<" "<<st.velocity<<" "<<st.probability<<" "<<st.microTicks<<"\n";
        }
        for(auto const&ev:pat.chopEvents){
            Tick recordedTick=ev.recordedTick;float recordedVelocity=ev.recordedVelocity;
            if(pat.chopQuantizeStrength==0.0f&&pat.chopHumanize==0.0f&&recordedTick==0&&ev.tick!=0){recordedTick=ev.tick;recordedVelocity=ev.velocity;}
            f<<"CHOP "<<ev.id<<" "<<recordedTick<<" "<<ev.tick<<" "<<ev.sampleId<<" "<<ev.sliceId<<" "<<recordedVelocity<<" "<<ev.velocity<<" "<<ev.pan<<"\n";
        }
    }
    f<<"END\n";
}

Project ProjectSerializer::load(const std::filesystem::path&path,bool loadAudio){
    std::ifstream f(path);if(!f)throw std::runtime_error("Cannot open project");Project p;std::string tag;f>>tag;if(tag!="FLOWDAW_PROJECT")throw std::runtime_error("Not a FLOWDAW project");int version=0;f>>version;if(version<1||version>7)throw std::runtime_error("Unsupported project version");
    while(f>>tag){
        if(tag=="NAME")f>>std::quoted(p.name);
        else if(tag=="SAMPLE_RATE")f>>p.sampleRate;
        else if(tag=="BPM")f>>p.transport.bpm;
        else if(tag=="PLAYHEAD")f>>p.transport.playheadTick;
        else if(tag=="MASTER"){std::size_t nfx=0;f>>p.master.volume;if(version>=3)f>>nfx;for(std::size_t i=0;i<nfx;++i)p.master.effects.push_back(loadEffect(f,tag,"MASTER_EFFECT"));}
        else if(tag=="SAMPLES"){
            std::size_t n{};f>>n;for(std::size_t i=0;i<n;++i){SampleAsset s;f>>tag;if(tag!="SAMPLE")throw std::runtime_error("Expected SAMPLE");f>>s.id>>std::quoted(s.name);std::string sp;f>>std::quoted(sp);s.path=sp;if(version>=2)f>>std::quoted(s.nativeKey);
                std::size_t nslices=0;if(version>=4)f>>s.detectedBpm>>s.bpmConfidence>>s.sourceSampleId>>s.timeRatio>>nslices;
                for(std::size_t j=0;j<nslices;++j){SampleSlice sl;f>>tag;if(tag!="SLICE")throw std::runtime_error("Expected SLICE");f>>sl.id>>std::quoted(sl.name)>>sl.startFrame>>sl.endFrame;if(version>=7)f>>sl.gain>>sl.pan>>sl.chokeGroup;s.slices.push_back(std::move(sl));}
                if(loadAudio&&s.sourceSampleId==0){if(!s.nativeKey.empty())s.audio=std::make_shared<AudioBuffer>(makeNativeDrum(s.nativeKey,p.sampleRate));else if(!s.path.empty()&&std::filesystem::exists(s.path))s.audio=std::make_shared<AudioBuffer>(WavFile::read(s.path));}
                p.samples.push_back(std::move(s));
            }
            if(loadAudio&&version>=4){for(auto&derived:p.samples){if(derived.sourceSampleId==0||derived.audio)continue;auto*source=p.findSample(derived.sourceSampleId);if(source&&source->audio&&derived.timeRatio>=0.5&&derived.timeRatio<=2.0)derived.audio=std::make_shared<AudioBuffer>(timeStretchWsola(*source->audio,derived.timeRatio));}}
        }
        else if(tag=="TRACKS"){
            std::size_t n{};f>>n;for(std::size_t i=0;i<n;++i){Track t;std::size_t nc=0,np=0,nfx=0;f>>tag;if(tag!="TRACK")throw std::runtime_error("Expected TRACK");f>>t.id>>std::quoted(t.name)>>t.mixer.volume>>t.mixer.pan>>t.mixer.mute>>t.mixer.solo>>nc;if(version>=2)f>>np;if(version>=3)f>>nfx;
                for(std::size_t j=0;j<nc;++j){Clip c;f>>tag;if(tag!="CLIP")throw std::runtime_error("Expected CLIP");f>>c.id>>c.sampleId>>c.startTick>>c.lengthTicks>>c.sourceStart>>c.sourceLength>>c.gain>>c.loop;t.clips.push_back(c);}for(std::size_t j=0;j<np;++j){PatternPlacement pc;f>>tag;if(tag!="PATCLIP")throw std::runtime_error("Expected PATCLIP");f>>pc.id>>pc.patternId>>pc.startTick>>pc.repeats;t.patternClips.push_back(pc);}for(std::size_t j=0;j<nfx;++j)t.mixer.effects.push_back(loadEffect(f,tag,"TRACK_EFFECT"));p.tracks.push_back(std::move(t));
            }
        }
        else if(tag=="PATTERNS"){
            std::size_t n{};f>>n;for(std::size_t i=0;i<n;++i){Pattern pat;f>>tag;if(tag!="PATTERN")throw std::runtime_error("Expected PATTERN");
                if(version==1){Tick oldLen{};f>>pat.id>>std::quoted(pat.name)>>oldLen;pat.stepCount=16;pat.stepsPerBeat=4;}
                else{std::size_t nl=0,nchop=0;f>>pat.id>>std::quoted(pat.name)>>pat.stepCount>>pat.stepsPerBeat;if(version>=3)f>>pat.swing>>pat.humanize;f>>nl;if(version>=5)f>>nchop;if(version>=6)f>>pat.chopQuantizeGridTicks>>pat.chopQuantizeStrength>>pat.chopHumanize;for(std::size_t l=0;l<nl;++l){DrumLane lane;std::size_t ns=0;f>>tag;if(tag!="LANE")throw std::runtime_error("Expected LANE");f>>std::quoted(lane.name)>>lane.sampleId>>lane.volume>>lane.pan;if(version>=3)f>>lane.mute>>lane.solo;f>>ns;lane.steps.reserve(ns);for(std::size_t s=0;s<ns;++s){StepEvent st;f>>tag;if(tag!="STEP")throw std::runtime_error("Expected STEP");f>>st.active>>st.velocity>>st.probability>>st.microTicks;lane.steps.push_back(st);}pat.lanes.push_back(std::move(lane));}for(std::size_t c=0;c<nchop;++c){ChopEvent ev;f>>tag;if(tag!="CHOP")throw std::runtime_error("Expected CHOP");if(version>=6){f>>ev.id>>ev.recordedTick>>ev.tick>>ev.sampleId>>ev.sliceId>>ev.recordedVelocity>>ev.velocity>>ev.pan;}else{f>>ev.tick>>ev.sampleId>>ev.sliceId>>ev.velocity>>ev.pan;ev.recordedTick=ev.tick;ev.recordedVelocity=ev.velocity;}pat.chopEvents.push_back(ev);}}
                p.patterns.push_back(std::move(pat));
            }
        }
        else if(tag=="END")break;else throw std::runtime_error("Unknown project token: "+tag);
    }
    p.formatVersion=7;return p;
}
}
