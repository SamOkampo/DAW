#include "flowdaw/Project.hpp"
namespace flowdaw {
SampleAsset* Project::findSample(Id id){ for(auto& s:samples) if(s.id==id) return &s; return nullptr; }
const SampleAsset* Project::findSample(Id id) const { for(auto const& s:samples) if(s.id==id) return &s; return nullptr; }
Track* Project::findTrack(Id id){ for(auto& t:tracks) if(t.id==id) return &t; return nullptr; }
Pattern* Project::findPattern(Id id){ for(auto& p:patterns) if(p.id==id) return &p; return nullptr; }
const Pattern* Project::findPattern(Id id) const { for(auto const& p:patterns) if(p.id==id) return &p; return nullptr; }
Bus* Project::findBus(Id id){ for(auto& b:buses) if(b.id==id) return &b; return nullptr; }
const Bus* Project::findBus(Id id) const { for(auto const& b:buses) if(b.id==id) return &b; return nullptr; }
}
