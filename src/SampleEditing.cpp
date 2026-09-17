#include "flowdaw/SampleEditing.hpp"
#include <algorithm>

namespace flowdaw {
bool moveSliceBoundary(SampleAsset& sample,std::size_t boundaryIndex,SampleIndex newFrame,SampleIndex minSliceFrames){
    if(boundaryIndex+1>=sample.slices.size())return false;
    minSliceFrames=std::max<SampleIndex>(1,minSliceFrames);
    auto&left=sample.slices[boundaryIndex];auto&right=sample.slices[boundaryIndex+1];
    const auto lo=left.startFrame+minSliceFrames;
    const auto hi=right.endFrame-minSliceFrames;
    if(lo>hi)return false;
    const auto frame=std::clamp(newFrame,lo,hi);
    left.endFrame=frame;right.startFrame=frame;return true;
}

bool insertSliceBoundary(SampleAsset& sample,SampleIndex frame,SampleIndex minSliceFrames){
    minSliceFrames=std::max<SampleIndex>(1,minSliceFrames);
    for(std::size_t i=0;i<sample.slices.size();++i){
        auto const current=sample.slices[i];
        if(frame<=current.startFrame||frame>=current.endFrame)continue;
        if(frame-current.startFrame<minSliceFrames||current.endFrame-frame<minSliceFrames)return false;
        SampleSlice left=current,right=current;left.id=nextId();right.id=nextId();
        left.name="Slice "+std::to_string(i+1);right.name="Slice "+std::to_string(i+2);
        left.endFrame=frame;right.startFrame=frame;
        sample.slices[i]=left;sample.slices.insert(sample.slices.begin()+static_cast<std::ptrdiff_t>(i+1),right);
        for(std::size_t n=0;n<sample.slices.size();++n)sample.slices[n].name="Slice "+std::to_string(n+1);
        return true;
    }
    return false;
}

bool removeSliceBoundary(SampleAsset& sample,std::size_t boundaryIndex){
    if(boundaryIndex+1>=sample.slices.size())return false;
    auto&left=sample.slices[boundaryIndex];auto const right=sample.slices[boundaryIndex+1];
    left.endFrame=right.endFrame;
    sample.slices.erase(sample.slices.begin()+static_cast<std::ptrdiff_t>(boundaryIndex+1));
    for(std::size_t n=0;n<sample.slices.size();++n)sample.slices[n].name="Slice "+std::to_string(n+1);
    return true;
}
}
