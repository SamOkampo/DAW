#include "flowdaw/Serialization.hpp"
#include "flowdaw/NativeDrums.hpp"
#include "flowdaw/Wav.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace flowdaw {
namespace {
std::string q(const std::string& s){ std::ostringstream o; o<<std::quoted(s); return o.str(); }
void saveEffects(std::ofstream& f,const char* tag,const std::vector<Effect>& effects){
    for(auto const& fx:effects) f<<tag<<" "<<fx.id<<" "<<q(fx.type)<<" "<<fx.enabled<<" "<<fx.value<<"\n";
}
Effect loadEffect(std::ifstream& f){ Effect fx; std::string tag; f>>tag>>fx.id>>std::quoted(fx.type)>>fx.enabled>>fx.value; return fx; }
}

void ProjectSerializer::save(const Project& p,const std::filesystem::path& path){
    std::ofstream f(path); if(!f) throw std::runtime_error("Cannot save project");
    f<<"FLOWDAW_PROJECT 3\n";
    f<<"NAME "<<q(p.name)<<"\nSAMPLE_RATE "<<p.sampleRate<<"\nBPM "<<std::setprecision(12)<<p.transport.bpm<<"\nPLAYHEAD "<<p.transport.playheadTick<<"\n";
    f<<"MASTER "<<p.master.volume<<" "<<p.master.effects.size()<<"\n";
    saveEffects(f,"MASTER_EFFECT",p.master.effects);
    f<<"SAMPLES "<<p.samples.size()<<"\n";
    for(auto const& s:p.samples) f<<"SAMPLE "<<s.id<<" "<<q(s.name)<<" "<<q(s.path.generic_string())<<" "<<q(s.nativeKey)<<"\n";
    f<<"TRACKS "<<p.tracks.size()<<"\n";
    for(auto const& t:p.tracks){
        f<<"TRACK "<<t.id<<" "<<q(t.name)<<" "<<t.mixer.volume<<" "<<t.mixer.pan<<" "<<t.mixer.mute<<" "<<t.mixer.solo<<" "<<t.clips.size()<<" "<<t.patternClips.size()<<" "<<t.mixer.effects.size()<<"\n";
        for(auto const& c:t.clips) f<<"CLIP "<<c.id<<" "<<c.sampleId<<" "<<c.startTick<<" "<<c.lengthTicks<<" "<<c.sourceStart<<" "<<c.sourceLength<<" "<<c.gain<<" "<<c.loop<<"\n";
        for(auto const& pc:t.patternClips) f<<"PATCLIP "<<pc.id<<" "<<pc.patternId<<" "<<pc.startTick<<" "<<pc.repeats<<"\n";
        saveEffects(f,"TRACK_EFFECT",t.mixer.effects);
    }
    f<<"PATTERNS "<<p.patterns.size()<<"\n";
    for(auto const& pat:p.patterns){
        f<<"PATTERN "<<pat.id<<" "<<q(pat.name)<<" "<<pat.stepCount<<" "<<pat.stepsPerBeat<<" "<<pat.swing<<" "<<pat.humanize<<" "<<pat.lanes.size()<<"\n";
        for(auto const& lane:pat.lanes){
            f<<"LANE "<<q(lane.name)<<" "<<lane.sampleId<<" "<<lane.volume<<" "<<lane.pan<<" "<<lane.mute<<" "<<lane.solo<<" "<<lane.steps.size()<<"\n";
            for(auto const& st:lane.steps) f<<"STEP "<<st.active<<" "<<st.velocity<<" "<<st.probability<<" "<<st.microTicks<<"\n";
        }
    }
    f<<"END\n";
}

Project ProjectSerializer::load(const std::filesystem::path& path,bool loadAudio){
    std::ifstream f(path); if(!f) throw std::runtime_error("Cannot open project");
    Project p; std::string tag; f>>tag; if(tag!="FLOWDAW_PROJECT") throw std::runtime_error("Not a FLOWDAW project");
    int version=0; f>>version; if(version<1||version>3) throw std::runtime_error("Unsupported project version"); p.formatVersion=3;
    while(f>>tag){
        if(tag=="NAME") f>>std::quoted(p.name);
        else if(tag=="SAMPLE_RATE") f>>p.sampleRate;
        else if(tag=="BPM") f>>p.transport.bpm;
        else if(tag=="PLAYHEAD") f>>p.transport.playheadTick;
        else if(tag=="MASTER") {
            std::size_t nfx=0; f>>p.master.volume; if(version>=3) f>>nfx;
            for(std::size_t i=0;i<nfx;++i) p.master.effects.push_back(loadEffect(f));
        }
        else if(tag=="SAMPLES") {
            std::size_t n{}; f>>n;
            for(std::size_t i=0;i<n;++i){
                SampleAsset s; f>>tag>>s.id>>std::quoted(s.name); std::string sp; f>>std::quoted(sp); s.path=sp; if(version>=2) f>>std::quoted(s.nativeKey);
                if(loadAudio){
                    if(!s.nativeKey.empty()) s.audio=std::make_shared<AudioBuffer>(makeNativeDrum(s.nativeKey,p.sampleRate));
                    else if(!s.path.empty() && std::filesystem::exists(s.path)) s.audio=std::make_shared<AudioBuffer>(WavFile::read(s.path));
                }
                p.samples.push_back(std::move(s));
            }
        }
        else if(tag=="TRACKS") {
            std::size_t n{}; f>>n;
            for(std::size_t i=0;i<n;++i){
                Track t; std::size_t nc{},np{},nfx{}; f>>tag>>t.id>>std::quoted(t.name)>>t.mixer.volume>>t.mixer.pan>>t.mixer.mute>>t.mixer.solo>>nc; if(version>=2) f>>np; if(version>=3) f>>nfx;
                for(std::size_t j=0;j<nc;++j){ Clip c; f>>tag>>c.id>>c.sampleId>>c.startTick>>c.lengthTicks>>c.sourceStart>>c.sourceLength>>c.gain>>c.loop; t.clips.push_back(c); }
                for(std::size_t j=0;j<np;++j){ PatternPlacement pc; f>>tag>>pc.id>>pc.patternId>>pc.startTick>>pc.repeats; t.patternClips.push_back(pc); }
                for(std::size_t j=0;j<nfx;++j) t.mixer.effects.push_back(loadEffect(f));
                p.tracks.push_back(std::move(t));
            }
        }
        else if(tag=="PATTERNS") {
            std::size_t n{}; f>>n;
            for(std::size_t i=0;i<n;++i){
                Pattern pat;
                if(version==1){ Tick oldLen{}; f>>tag>>pat.id>>std::quoted(pat.name)>>oldLen; pat.stepCount=16; pat.stepsPerBeat=4; }
                else {
                    std::size_t nl{};
                    f>>tag>>pat.id>>std::quoted(pat.name)>>pat.stepCount>>pat.stepsPerBeat;
                    if(version>=3) f>>pat.swing>>pat.humanize;
                    f>>nl;
                    for(std::size_t l=0;l<nl;++l){
                        DrumLane lane; std::size_t ns{}; f>>tag>>std::quoted(lane.name)>>lane.sampleId>>lane.volume>>lane.pan;
                        if(version>=3) f>>lane.mute>>lane.solo;
                        f>>ns; lane.steps.reserve(ns);
                        for(std::size_t s=0;s<ns;++s){ StepEvent st; f>>tag>>st.active>>st.velocity>>st.probability>>st.microTicks; lane.steps.push_back(st); }
                        pat.lanes.push_back(std::move(lane));
                    }
                }
                p.patterns.push_back(std::move(pat));
            }
        }
        else if(tag=="END") break;
        else throw std::runtime_error("Unknown project token: "+tag);
    }
    return p;
}
}
