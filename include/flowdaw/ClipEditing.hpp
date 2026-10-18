#pragma once

#include "flowdaw/Project.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace flowdaw {

// Control-thread-only, non-destructive editing of non-looping audio clips.
// All positions and source lengths use existing .flow v11 fields.
inline SampleIndex sourceFramesForTicks(const Clip& clip, Tick ticks) {
    if(clip.lengthTicks<=0||clip.sourceLength<=0)return 0;
    const auto fraction=std::clamp(static_cast<long double>(ticks)
        /static_cast<long double>(clip.lengthTicks),0.0L,1.0L);
    return static_cast<SampleIndex>(std::llround(
        fraction*static_cast<long double>(clip.sourceLength)));
}

inline std::optional<std::pair<Clip,Clip>> splitAudioClip(const Clip& original,Tick splitTick) {
    if(original.loop||original.lengthTicks<=1||original.sourceLength<=1)
        return std::nullopt;
    if(splitTick<=original.startTick||splitTick>=original.startTick+original.lengthTicks)
        return std::nullopt;
    const auto elapsed=splitTick-original.startTick;
    const auto frames=sourceFramesForTicks(original,elapsed);
    if(frames<=0||frames>=original.sourceLength)return std::nullopt;
    Clip left=original,right=original;
    left.lengthTicks=elapsed;
    left.sourceLength=frames;
    right.id=nextId();
    right.startTick=splitTick;
    right.lengthTicks=original.lengthTicks-elapsed;
    right.sourceStart=original.sourceStart+frames;
    right.sourceLength=original.sourceLength-frames;
    return std::pair{left,right};
}

inline bool trimAudioClipLeft(Clip& clip,Tick newStartTick) {
    if(clip.loop||newStartTick<=clip.startTick||
       newStartTick>=clip.startTick+clip.lengthTicks)return false;
    const auto elapsed=newStartTick-clip.startTick;
    const auto used=sourceFramesForTicks(clip,elapsed);
    if(used<=0||used>=clip.sourceLength)return false;
    clip.startTick=newStartTick;
    clip.lengthTicks-=elapsed;
    clip.sourceStart+=used;
    clip.sourceLength-=used;
    return true;
}

inline bool trimAudioClipRight(Clip& clip,Tick newEndTick) {
    if(clip.loop||newEndTick<=clip.startTick||
       newEndTick>=clip.startTick+clip.lengthTicks)return false;
    const auto elapsed=newEndTick-clip.startTick;
    const auto kept=sourceFramesForTicks(clip,elapsed);
    if(kept<=0||kept>=clip.sourceLength)return false;
    clip.lengthTicks=elapsed;
    clip.sourceLength=kept;
    return true;
}
} // namespace flowdaw
