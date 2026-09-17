#include "flowdaw/Workflow.hpp"
#include "flowdaw/PluginHost.hpp"
#include <algorithm>
#include <cctype>
#include <set>

namespace flowdaw {
namespace {
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
bool containsInsensitive(const std::string&a,const std::string&b){return lower(a).find(lower(b))!=std::string::npos;}
}
std::vector<ProjectHealthIssue> validateProject(const Project&p){
 std::vector<ProjectHealthIssue> out;std::set<Id> ids;
 auto unique=[&](Id id,const std::string&what){if(!id||!ids.insert(id).second)out.push_back({"duplicate-id",what+" has a missing or duplicate stable ID",id,true});};
 for(auto const&s:p.samples)unique(s.id,"Sample "+s.name);for(auto const&pat:p.patterns)unique(pat.id,"Pattern "+pat.name);for(auto const&b:p.buses)unique(b.id,"Bus "+b.name);
 for(auto const&t:p.tracks){unique(t.id,"Track "+t.name);if(t.outputBusId&&!p.findBus(t.outputBusId))out.push_back({"broken-route",t.name+" routes to a missing bus",t.id,true});for(auto const&c:t.clips)if(!p.findSample(c.sampleId))out.push_back({"missing-sample",t.name+" contains a clip with a missing sample",c.id,true});for(auto const&take:t.takes)if(!p.findSample(take.sampleId))out.push_back({"missing-take-audio",t.name+" contains a take with missing audio",take.id,true});for(auto const&s:t.sends)if(!p.findBus(s.busId))out.push_back({"broken-send",t.name+" contains a send to a missing bus",s.id,true});for(auto const&pl:t.mixer.plugins)if(pl.format!="builtin"&&pl.format!="vst3"&&pl.format!="au")out.push_back({"plugin-format","Unknown plugin format on "+t.name+": "+pl.format,pl.id,true});}
 for(auto const&pat:p.patterns){for(auto const&lane:pat.lanes)if(lane.sampleId&&!p.findSample(lane.sampleId))out.push_back({"missing-drum-sample",pat.name+" references a missing drum sample",pat.id,true});for(auto const&e:pat.chopEvents)if(!p.findSample(e.sampleId))out.push_back({"missing-chop-sample",pat.name+" contains a chop with a missing sample",e.id,true});}
 if(p.master.volume>1.25f)out.push_back({"hot-master","Master volume is above 125%; clipping risk is elevated",0,false});if(p.transport.bpm<20.0||p.transport.bpm>400.0)out.push_back({"bpm-range","Project BPM is outside the supported musical range",0,true});return out;
}
std::vector<WorkflowCommand> workflowCommands(const std::string&q){
 std::vector<WorkflowCommand> all={{"master-headroom","Set master headroom","Set the master fader to 85%."},{"create-mix-bus","Create MIX BUS","Add a new bus without changing track routing."},{"add-master-gain","Add FLOW Gain","Insert the native gain plugin on the master."},{"add-master-softclip","Add FLOW Soft Clip","Insert a gentle native soft clipper on the master."},{"clear-solos","Clear all solos","Disable solo on every track and bus."},{"arm-vocal","Arm vocal track","Find or create VOCAL, arm it and disarm the other tracks."},{"bypass-external","Bypass external plugins","Bypass VST3/AU slots while keeping their state intact."}};if(q.empty())return all;std::vector<WorkflowCommand> out;for(auto const&c:all)if(containsInsensitive(c.id,q)||containsInsensitive(c.title,q)||containsInsensitive(c.description,q))out.push_back(c);return out;
}
bool executeWorkflowCommand(Project&p,const std::string&id,Id,std::string&result){
 if(id=="master-headroom"){p.master.volume=.85f;result="Master set to 85%";return true;}
 if(id=="create-mix-bus"){Bus b;b.name="MIX BUS";p.buses.push_back(b);result="MIX BUS created";return true;}
 if(id=="add-master-gain"){p.master.plugins.push_back(makeBuiltinPlugin("flow.gain"));result="FLOW Gain added to master";return true;}
 if(id=="add-master-softclip"){auto x=makeBuiltinPlugin("flow.softclip");setPluginParameter(x,"drive",.12f);x.wet=.65f;p.master.plugins.push_back(std::move(x));result="FLOW Soft Clip added to master";return true;}
 if(id=="clear-solos"){for(auto&t:p.tracks)t.mixer.solo=false;for(auto&b:p.buses)b.mixer.solo=false;result="All solos cleared";return true;}
 if(id=="arm-vocal"){Track* vocal=nullptr;for(auto&t:p.tracks){t.armed=false;if(t.name=="VOCAL")vocal=&t;}if(!vocal){Track t;t.name="VOCAL";p.tracks.push_back(t);vocal=&p.tracks.back();}vocal->armed=true;result="VOCAL armed";return true;}
 if(id=="bypass-external"){int n=0;auto bypass=[&](auto&plugins){for(auto&pl:plugins)if(pl.format=="vst3"||pl.format=="au"){pl.bypass=true;++n;}};bypass(p.master.plugins);for(auto&t:p.tracks)bypass(t.mixer.plugins);for(auto&b:p.buses)bypass(b.mixer.plugins);result="Bypassed "+std::to_string(n)+" external plugin slots";return n>0;}
 result="Unknown workflow command";return false;
}
}
