#include "flowdaw/ClipEditing.hpp"
#include "flowdaw/Serialization.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>

using namespace flowdaw;

int main() {
    Clip original; original.startTick=960; original.lengthTicks=3840;
    original.sourceStart=1200; original.sourceLength=96000;
    original.sampleId=42;original.gain=0.8f;

    const auto middle=original.startTick+1920;
    auto split=splitAudioClip(original,middle);
    assert(split.has_value());
    const auto&left=split->first;
    const auto&right=split->second;
    assert(left.id==original.id && right.id!=original.id);
    assert(left.startTick==960 && left.lengthTicks==1920);
    assert(right.startTick==middle && right.lengthTicks==1920);
    assert(left.sourceStart==1200 && left.sourceLength==48000);
    assert(right.sourceStart==49200 && right.sourceLength==48000);
    assert(left.sourceStart+left.sourceLength==right.sourceStart);
    assert(right.sourceStart+right.sourceLength==original.sourceStart+original.sourceLength);
    assert(!splitAudioClip(original,original.startTick));
    assert(!splitAudioClip(original,original.startTick+original.lengthTicks));
    auto looping=original;looping.loop=true;
    assert(!splitAudioClip(looping,middle));

    Clip trim=original;
    assert(trimAudioClipLeft(trim,original.startTick+960));
    assert(trim.startTick==1920&&trim.lengthTicks==2880);
    assert(trim.sourceStart==25200&&trim.sourceLength==72000);
    assert(trimAudioClipRight(trim,3840));
    assert(trim.startTick==1920&&trim.lengthTicks==1920);
    assert(trim.sourceStart==25200&&trim.sourceLength==48000);
    assert(!trimAudioClipRight(trim,trim.startTick));
    assert(!trimAudioClipLeft(looping,looping.startTick+960));

    Project p;Track t;t.clips.push_back(split->first);t.clips.push_back(split->second);
    p.tracks.push_back(t);
    const auto path=std::filesystem::temp_directory_path()/
        ("flowdaw-clip-edit-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".flow");
    ProjectSerializer::save(p,path);
    const auto roundtrip=ProjectSerializer::load(path,false);
    assert(roundtrip.tracks.size()==1&&roundtrip.tracks[0].clips.size()==2);
    const auto&serializedLeft=roundtrip.tracks[0].clips[0];
    const auto&serializedRight=roundtrip.tracks[0].clips[1];
    assert(serializedLeft.lengthTicks==left.lengthTicks);
    assert(serializedRight.sourceStart==right.sourceStart);
    assert(serializedRight.sourceLength==right.sourceLength);
    std::error_code ec;std::filesystem::remove(path,ec);
    return 0;
}
