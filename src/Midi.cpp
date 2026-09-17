#include "flowdaw/Midi.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace flowdaw {
double midiNoteFrequency(int pitch){return 440.0*std::pow(2.0,(static_cast<double>(pitch)-69.0)/12.0);}
std::string midiNoteName(int pitch){
    static constexpr std::array<const char*,12> names={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    pitch=std::clamp(pitch,0,127);return std::string(names[static_cast<std::size_t>(pitch%12)])+std::to_string(pitch/12-1);
}
Tick snapMidiTick(Tick tick,Tick gridTicks){if(gridTicks<=0)return std::max<Tick>(0,tick);tick=std::max<Tick>(0,tick);return ((tick+gridTicks/2)/gridTicks)*gridTicks;}
std::vector<int> scalePitchClasses(int root,const std::string&type){
    root=((root%12)+12)%12;std::vector<int> intervals;
    if(type=="major")intervals={0,2,4,5,7,9,11};
    else if(type=="minor_pentatonic")intervals={0,3,5,7,10};
    else if(type=="major_pentatonic")intervals={0,2,4,7,9};
    else intervals={0,2,3,5,7,8,10};
    std::vector<int> out;out.reserve(intervals.size());for(int i:intervals)out.push_back((root+i)%12);return out;
}
bool pitchInScale(int pitch,int root,const std::string&type){auto pcs=scalePitchClasses(root,type);const int pc=((pitch%12)+12)%12;return std::find(pcs.begin(),pcs.end(),pc)!=pcs.end();}
}
