#include "flowdaw/ProductionAssistant.hpp"
#include "flowdaw/PluginHost.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace flowdaw {
std::string buildProjectContext(const Project&p){
 std::ostringstream o;o<<"FLOWDAW_CONTEXT v1\nproject="<<p.name<<"\nbpm="<<p.transport.bpm<<"\nsample_rate="<<p.sampleRate<<"\ntracks="<<p.tracks.size()<<"\npatterns="<<p.patterns.size()<<"\nsamples="<<p.samples.size()<<"\nbuses="<<p.buses.size()<<"\nautomation_lanes="<<p.automation.size()<<"\nmaster_plugins="<<p.master.plugins.size()<<"\n";
 for(auto const&t:p.tracks)o<<"track id="<<t.id<<" name=\""<<t.name<<"\" vol="<<t.mixer.volume<<" pan="<<t.mixer.pan<<" clips="<<t.clips.size()<<" patterns="<<t.patternClips.size()<<" takes="<<t.takes.size()<<" plugins="<<t.mixer.plugins.size()<<" output_bus="<<t.outputBusId<<"\n";
 for(auto const&pat:p.patterns)o<<"pattern id="<<pat.id<<" name=\""<<pat.name<<"\" steps="<<pat.stepCount<<" lanes="<<pat.lanes.size()<<" midi="<<pat.midiNotes.size()<<" chops="<<pat.chopEvents.size()<<" swing="<<pat.swing<<" humanize="<<pat.humanize<<"\n";
 for(auto const&s:p.samples)if(s.detectedBpm>0)o<<"sample id="<<s.id<<" name=\""<<s.name<<"\" bpm="<<s.detectedBpm<<" confidence="<<s.bpmConfidence<<"\n";
 return o.str();
}

std::vector<AssistantSuggestion> analyzeProductionContext(const Project&p){
 std::vector<AssistantSuggestion> out;
 if(p.master.volume>0.95f)out.push_back({"master-headroom","Leave master headroom","The master fader is close to unity. Lowering it slightly gives later processing and export more margin.","Set master to 85%",AssistantAction::SetMasterHeadroom,0,0.85f,90});
 if(p.tracks.size()>=3&&p.buses.empty())out.push_back({"mix-bus","Create a mix bus","This project has several tracks but no bus. A dedicated bus gives you one place for shared processing and automation.","Create MIX BUS",AssistantAction::CreateMixBus,0,0.0f,70});
 for(auto const&t:p.tracks)if(t.mixer.volume>1.15f){out.push_back({"track-trim-"+std::to_string(t.id),"Trim a hot track",t.name+" is above 115% gain. Trimming before the master can reduce accidental clipping while keeping the balance editable.","Trim to 100%",AssistantAction::TrimTrack,t.id,1.0f,85});break;}
 for(auto const&pat:p.patterns)if(!pat.lanes.empty()&&pat.swing<0.01f&&pat.humanize<0.01f){out.push_back({"groove-"+std::to_string(pat.id),"Add subtle groove",pat.name+" is perfectly straight. A small amount of swing and deterministic humanize can create movement without changing the recorded source events.","Apply subtle groove",AssistantAction::GroovePattern,pat.id,0.0f,55});break;}
 if(!p.tracks.empty()&&p.master.plugins.empty())out.push_back({"softclip-master","Try gentle master soft clipping","The master rack is empty. FLOW Soft Clip can catch short peaks; it is added conservatively and remains bypassable/removable.","Add FLOW Soft Clip",AssistantAction::AddMasterSoftClip,0,0.0f,45});
 for(auto const&t:p.tracks)if(t.takes.size()>1){out.push_back({"comp-review-"+std::to_string(t.id),"Review alternate takes",t.name+" has "+std::to_string(t.takes.size())+" takes. The active take is non-destructive, so compare them before exporting.","Review takes",AssistantAction::None,t.id,0.0f,35});break;}
 for(auto const&s:p.samples)if(s.detectedBpm>0.0&&s.bpmConfidence>=0.55f&&std::abs(s.detectedBpm-p.transport.bpm)>8.0){out.push_back({"tempo-check-"+std::to_string(s.id),"Check sample tempo",s.name+" was detected near "+std::to_string(static_cast<int>(std::lround(s.detectedBpm)))+" BPM while the project is "+std::to_string(static_cast<int>(std::lround(p.transport.bpm)))+" BPM. Consider Match BPM before arranging it.","Open Smart Sampling",AssistantAction::None,s.id,0.0f,50});break;}
 std::stable_sort(out.begin(),out.end(),[](auto const&a,auto const&b){return a.priority>b.priority;});return out;
}

bool applyAssistantSuggestion(Project&p,const AssistantSuggestion&s,std::string&result){
 switch(s.action){
  case AssistantAction::SetMasterHeadroom:p.master.volume=std::clamp(s.value,0.0f,2.0f);result="Master headroom applied";return true;
  case AssistantAction::CreateMixBus:{Bus b;b.name="MIX BUS";p.buses.push_back(b);result="MIX BUS created; routing remains under your control";return true;}
  case AssistantAction::TrimTrack:{auto*t=p.findTrack(s.targetId);if(!t){result="Target track no longer exists";return false;}t->mixer.volume=std::clamp(s.value,0.0f,2.0f);result=t->name+" trimmed";return true;}
  case AssistantAction::AddMasterSoftClip:{for(auto const&x:p.master.plugins)if(x.identifier=="flow.softclip"){result="FLOW Soft Clip already exists on master";return false;}auto plug=makeBuiltinPlugin("flow.softclip");setPluginParameter(plug,"drive",0.12f);plug.wet=0.65f;p.master.plugins.push_back(std::move(plug));result="FLOW Soft Clip added to master";return true;}
  case AssistantAction::AddTrackGain:{auto*t=p.findTrack(s.targetId);if(!t){result="Target track no longer exists";return false;}t->mixer.plugins.push_back(makeBuiltinPlugin("flow.gain"));result="FLOW Gain added to "+t->name;return true;}
  case AssistantAction::GroovePattern:{auto*pat=p.findPattern(s.targetId);if(!pat){result="Target pattern no longer exists";return false;}pat->swing=std::max(pat->swing,0.12f);pat->humanize=std::max(pat->humanize,0.06f);result="Subtle deterministic groove applied";return true;}
  case AssistantAction::None:result="This suggestion is informational and makes no automatic edit";return false;
 }
 result="Unknown assistant action";return false;
}
}
