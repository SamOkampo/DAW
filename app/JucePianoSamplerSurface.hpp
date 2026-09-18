#pragma once
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Midi.hpp"
#include "flowdaw/Project.hpp"
#include "flowdaw/SampleEditing.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>

namespace flowdaw::juceui {

class PianoRollComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;

    PianoRollComponent(Project&project,CommitFn commit):project_(project),commit_(std::move(commit)){
        setWantsKeyboardFocus(true);
    }

    void setPatternId(Id id){patternId_=id;selectedNoteId_=0;dragging_=false;repaint();}
    Id patternId()const{return patternId_;}

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff0f1115));
        auto*p=pattern();
        if(!p){g.setColour(juce::Colour(0xff9aa0ad));g.drawText("Select a MIDI pattern",getLocalBounds(),juce::Justification::centred);return;}
        const auto grid=gridBounds();const Tick total=visibleTicks(*p);
        g.setColour(juce::Colour(0xff171a20));g.fillRect(grid);

        for(int row=0;row<kRows;++row){
            const int pitch=kHighPitch-row;const int y=grid.getY()+row*grid.getHeight()/kRows;const int y2=grid.getY()+(row+1)*grid.getHeight()/kRows;
            const bool inScale=pitchInScale(pitch,p->scaleRoot,p->scaleType);
            g.setColour(inScale?juce::Colour(0xff1d2424):juce::Colour(0xff181a20));g.fillRect(grid.getX(),y,grid.getWidth(),std::max(1,y2-y-1));
            g.setColour(juce::Colour(0xff2a2d34));g.drawHorizontalLine(y,static_cast<float>(grid.getX()),static_cast<float>(grid.getRight()));
            if((pitch%12)==0){g.setColour(juce::Colour(0xffdfe2e8));g.setFont(10.5f);g.drawFittedText(midiNoteName(pitch),4,y,kKeyboardWidth-8,std::max(1,y2-y),juce::Justification::centredLeft,1);}
        }

        const Tick step=std::max<Tick>(1,p->midiGridTicks);
        for(Tick tick=0;tick<=total;tick+=step){
            const int x=xForTick(tick,total);const bool beat=(tick%kPPQ)==0;
            g.setColour(beat?juce::Colour(0xff414650):juce::Colour(0xff292d35));g.drawVerticalLine(x,static_cast<float>(grid.getY()),static_cast<float>(grid.getBottom()));
        }

        for(auto const&note:p->midiNotes){
            auto r=noteRect(note,*p);const bool selected=note.id==selectedNoteId_;
            g.setColour(selected?juce::Colour(0xff8e78ff):juce::Colour(0xff6550dc));g.fillRoundedRectangle(r,3.0f);
            g.setColour(juce::Colours::white.withAlpha(0.9f));g.setFont(10.0f);g.drawFittedText(midiNoteName(note.pitch),r.toNearestInt().reduced(4,0),juce::Justification::centredLeft,1);
            if(selected){g.setColour(juce::Colour(0xffffc06a));g.drawRoundedRectangle(r,3.0f,1.5f);}
        }

        g.setColour(juce::Colour(0xffaeb4c0));g.setFont(11.0f);
        g.drawText(juce::String(p->name)+"  •  "+juce::String(p->scaleType)+"  •  grid "+gridLabel(step),kKeyboardWidth,2,std::max(1,getWidth()-kKeyboardWidth),18,juce::Justification::centredLeft,false);
        g.drawText("Click empty: add • Drag: move • Drag right edge: resize • Delete: remove",4,getHeight()-18,getWidth()-8,16,juce::Justification::centredLeft,false);
    }

    void mouseDown(const juce::MouseEvent&e)override{
        grabKeyboardFocus();auto*p=pattern();if(!p||!gridBounds().contains(e.getPosition()))return;
        const Id hit=noteAt(e.position,*p);
        if(hit==0){
            Project before=project_;MidiNote note;note.startTick=tickAtX(e.x,*p);note.lengthTicks=std::max<Tick>(1,p->midiDefaultLengthTicks);note.pitch=pitchAtY(e.y);note.velocity=0.9f;p->midiNotes.push_back(note);selectedNoteId_=note.id;
            if(commit_)commit_(std::move(before),"Add MIDI note");repaint();return;
        }
        selectedNoteId_=hit;auto*note=selectedNote();if(!note)return;
        before_=project_;dragging_=true;dragStartX_=e.x;dragStartY_=e.y;originalStart_=note->startTick;originalLength_=note->lengthTicks;originalPitch_=note->pitch;
        const auto r=noteRect(*note,*p);resizing_=e.position.x>=r.getRight()-7.0f;repaint();
    }

    void mouseDrag(const juce::MouseEvent&e)override{
        if(!dragging_)return;auto*p=pattern();auto*note=selectedNote();if(!p||!note)return;
        const Tick total=visibleTicks(*p);const int width=std::max(1,gridBounds().getWidth());
        const Tick deltaRaw=static_cast<Tick>(std::llround(static_cast<double>(e.x-dragStartX_)*static_cast<double>(total)/width));
        const Tick grid=std::max<Tick>(1,p->midiGridTicks);
        const Tick delta=static_cast<Tick>(std::llround(static_cast<double>(deltaRaw)/grid))*grid;
        if(resizing_)note->lengthTicks=std::max<Tick>(grid,originalLength_+delta);
        else{
            note->startTick=std::max<Tick>(0,originalStart_+delta);
            const int rowHeight=std::max(1,gridBounds().getHeight()/kRows);const int pitchDelta=-(e.y-dragStartY_)/rowHeight;
            note->pitch=std::clamp(originalPitch_+pitchDelta,kLowPitch,kHighPitch);
        }
        repaint();
    }

    void mouseUp(const juce::MouseEvent&)override{
        if(!dragging_)return;dragging_=false;auto*note=selectedNote();bool changed=false;
        if(note)changed=note->startTick!=originalStart_||note->lengthTicks!=originalLength_||note->pitch!=originalPitch_;
        if(changed&&commit_)commit_(std::move(before_),resizing_?"Resize MIDI note":"Move MIDI note");resizing_=false;repaint();
    }

    bool keyPressed(const juce::KeyPress&key)override{
        if(key!=juce::KeyPress::deleteKey&&key!=juce::KeyPress::backspaceKey)return false;
        auto*p=pattern();if(!p||selectedNoteId_==0)return false;
        auto it=std::find_if(p->midiNotes.begin(),p->midiNotes.end(),[&](auto const&n){return n.id==selectedNoteId_;});if(it==p->midiNotes.end())return false;
        Project before=project_;p->midiNotes.erase(it);selectedNoteId_=0;if(commit_)commit_(std::move(before),"Delete MIDI note");repaint();return true;
    }

private:
    static constexpr int kKeyboardWidth=64,kHeader=22,kFooter=20,kLowPitch=36,kHighPitch=83,kRows=kHighPitch-kLowPitch+1;

    Pattern*pattern(){return project_.findPattern(patternId_);}
    const Pattern*pattern()const{return project_.findPattern(patternId_);}
    MidiNote*selectedNote(){auto*p=pattern();if(!p)return nullptr;for(auto&n:p->midiNotes)if(n.id==selectedNoteId_)return &n;return nullptr;}
    juce::Rectangle<int>gridBounds()const{return{kKeyboardWidth,kHeader,std::max(1,getWidth()-kKeyboardWidth),std::max(1,getHeight()-kHeader-kFooter)};}
    static Tick visibleTicks(const Pattern&p){return std::max<Tick>(p.lengthTicks(),kPPQ*4);}
    int xForTick(Tick tick,Tick total)const{auto r=gridBounds();return r.getX()+static_cast<int>(std::llround(static_cast<double>(tick)/std::max<Tick>(1,total)*r.getWidth()));}
    Tick tickAtX(int x,const Pattern&p)const{auto r=gridBounds();const double u=std::clamp(static_cast<double>(x-r.getX())/std::max(1,r.getWidth()),0.0,1.0);return snapMidiTick(static_cast<Tick>(std::llround(u*visibleTicks(p))),std::max<Tick>(1,p.midiGridTicks));}
    int pitchAtY(int y)const{auto r=gridBounds();const double u=std::clamp(static_cast<double>(y-r.getY())/std::max(1,r.getHeight()),0.0,0.999999);return std::clamp(kHighPitch-static_cast<int>(u*kRows),kLowPitch,kHighPitch);}
    juce::Rectangle<float>noteRect(const MidiNote&n,const Pattern&p)const{
        auto r=gridBounds();const Tick total=visibleTicks(p);const int row=kHighPitch-std::clamp(n.pitch,kLowPitch,kHighPitch);const int y=r.getY()+row*r.getHeight()/kRows;const int y2=r.getY()+(row+1)*r.getHeight()/kRows;const int x=xForTick(n.startTick,total);const int x2=xForTick(n.startTick+std::max<Tick>(1,n.lengthTicks),total);
        return{static_cast<float>(x+1),static_cast<float>(y+1),static_cast<float>(std::max(5,x2-x-2)),static_cast<float>(std::max(5,y2-y-2))};
    }
    Id noteAt(juce::Point<float>point,const Pattern&p)const{for(auto it=p.midiNotes.rbegin();it!=p.midiNotes.rend();++it)if(noteRect(*it,p).contains(point))return it->id;return 0;}
    static juce::String gridLabel(Tick grid){if(grid==kPPQ/2)return"1/8";if(grid==kPPQ/4)return"1/16";if(grid==kPPQ/8)return"1/32";return juce::String(static_cast<int>(grid));}

    Project&project_;CommitFn commit_;Id patternId_=0,selectedNoteId_=0;bool dragging_=false,resizing_=false;int dragStartX_=0,dragStartY_=0;Tick originalStart_=0,originalLength_=0;int originalPitch_=60;Project before_;
};

class SamplerComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;
    using TriggerFn=std::function<void(Id,Id)>;

    SamplerComponent(Project&project,AudioEngine&engine,CommitFn commit,TriggerFn trigger={}):project_(project),engine_(engine),commit_(std::move(commit)),trigger_(std::move(trigger)){
        setWantsKeyboardFocus(true);
    }

    void setSampleId(Id id){sampleId_=id;selectedSliceId_=0;boundaryDrag_=-1;bank_=0;repaint();}
    Id sampleId()const{return sampleId_;}
    void previousBank(){bank_=std::max(0,bank_-1);repaint();}
    void nextBank(){auto*s=sample();if(!s)return;const int banks=std::max(1,(static_cast<int>(s->slices.size())+15)/16);bank_=std::min(banks-1,bank_+1);repaint();}
    int bank()const{return bank_;}

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff0f1115));auto*s=sample();
        if(!s||!s->audio){g.setColour(juce::Colour(0xff9aa0ad));g.drawText("Select an audio sample",getLocalBounds(),juce::Justification::centred);return;}
        auto wave=waveBounds();g.setColour(juce::Colour(0xff171a20));g.fillRoundedRectangle(wave.toFloat(),5.0f);
        const auto frames=s->audio->frames();const int channels=std::max(1,s->audio->channels);const float mid=static_cast<float>(wave.getCentreY());const float amp=wave.getHeight()*0.43f;
        g.setColour(juce::Colour(0xff4f8bd8));
        for(int x=0;x<wave.getWidth();++x){
            const SampleIndex a=static_cast<SampleIndex>((static_cast<double>(x)/std::max(1,wave.getWidth()))*frames);const SampleIndex b=std::min<SampleIndex>(frames,std::max<SampleIndex>(a+1,static_cast<SampleIndex>((static_cast<double>(x+1)/std::max(1,wave.getWidth()))*frames)));
            float peak=0.0f;const SampleIndex span=std::max<SampleIndex>(1,b-a);const SampleIndex step=std::max<SampleIndex>(1,span/24);
            for(SampleIndex f=a;f<b;f+=step){float v=0.0f;for(int c=0;c<channels;++c)v+=std::abs(s->audio->interleaved[static_cast<std::size_t>(f*channels+c)]);peak=std::max(peak,v/channels);}
            const float h=std::clamp(peak,0.0f,1.0f)*amp;g.drawVerticalLine(wave.getX()+x,mid-h,mid+h);
        }
        g.setColour(juce::Colour(0xff5a5e69));g.drawHorizontalLine(wave.getCentreY(),static_cast<float>(wave.getX()),static_cast<float>(wave.getRight()));

        for(std::size_t i=0;i<s->slices.size();++i){
            auto const&sl=s->slices[i];const int x=xForFrame(sl.startFrame,*s);if(i>0){g.setColour(juce::Colour(0xffffc06a));g.drawVerticalLine(x,static_cast<float>(wave.getY()),static_cast<float>(wave.getBottom()));}
            if(sl.id==selectedSliceId_){const int x2=xForFrame(sl.endFrame,*s);g.setColour(juce::Colour(0x356550dc));g.fillRect(x,wave.getY(),std::max(1,x2-x),wave.getHeight());}
        }

        g.setColour(juce::Colour(0xffdfe2e8));g.setFont(12.0f);
        const auto bpm=s->detectedBpm>0.0?"  •  "+juce::String(s->detectedBpm,1)+" BPM":juce::String{};
        g.drawText(juce::String(s->name)+bpm,6,2,getWidth()-12,18,juce::Justification::centredLeft,false);

        auto pads=padBounds();const int start=bank_*16;
        for(int cell=0;cell<16;++cell){const int index=start+cell;const int col=cell%4,row=cell/4;auto r=juce::Rectangle<int>(pads.getX()+col*pads.getWidth()/4,pads.getY()+row*pads.getHeight()/4,pads.getWidth()/4-4,pads.getHeight()/4-4).reduced(2);
            if(index>=static_cast<int>(s->slices.size())){g.setColour(juce::Colour(0xff1a1c21));g.fillRoundedRectangle(r.toFloat(),5.0f);continue;}
            auto const&sl=s->slices[static_cast<std::size_t>(index)];g.setColour(sl.id==selectedSliceId_?juce::Colour(0xff8e78ff):juce::Colour(0xff303640));g.fillRoundedRectangle(r.toFloat(),5.0f);g.setColour(juce::Colours::white);g.setFont(10.5f);g.drawFittedText(juce::String(index+1)+"  "+juce::String(sl.name),r.reduced(5,2),juce::Justification::centredLeft,1);
        }
        g.setColour(juce::Colour(0xffaeb4c0));g.setFont(10.5f);g.drawText("Bank "+juce::String(bank_+1)+" • click waveform/pad: preview • drag marker: move boundary • double-click: split • Delete: merge previous",6,getHeight()-18,getWidth()-12,16,juce::Justification::centredLeft,false);
    }

    void mouseDown(const juce::MouseEvent&e)override{
        grabKeyboardFocus();auto*s=sample();if(!s||!s->audio)return;
        if(padBounds().contains(e.getPosition())){selectPadAt(e.getPosition(),*s);return;}
        if(!waveBounds().contains(e.getPosition()))return;
        const int boundary=boundaryAtX(e.x,*s);if(boundary>=0){before_=project_;boundaryDrag_=boundary;originalBoundary_=s->slices[static_cast<std::size_t>(boundary)].endFrame;return;}
        const SampleIndex frame=frameAtX(e.x,*s);for(auto const&sl:s->slices)if(frame>=sl.startFrame&&frame<sl.endFrame){selectedSliceId_=sl.id;preview(sl,*s);repaint();break;}
    }

    void mouseDrag(const juce::MouseEvent&e)override{
        auto*s=sample();if(!s||boundaryDrag_<0)return;const auto frame=frameAtX(e.x,*s);moveSliceBoundary(*s,static_cast<std::size_t>(boundaryDrag_),frame,64);repaint();
    }

    void mouseUp(const juce::MouseEvent&)override{
        auto*s=sample();if(!s||boundaryDrag_<0)return;const auto now=s->slices[static_cast<std::size_t>(boundaryDrag_)].endFrame;const bool changed=now!=originalBoundary_;boundaryDrag_=-1;if(changed&&commit_)commit_(std::move(before_),"Move slice boundary");repaint();
    }

    void mouseDoubleClick(const juce::MouseEvent&e)override{
        auto*s=sample();if(!s||!s->audio||!waveBounds().contains(e.getPosition()))return;const auto frame=frameAtX(e.x,*s);Project before=project_;if(insertSliceBoundary(*s,frame,64)){if(commit_)commit_(std::move(before),"Split sample slice");repaint();}
    }

    bool keyPressed(const juce::KeyPress&key)override{
        auto*s=sample();if(!s)return false;
        const auto ch=static_cast<char>(std::tolower(static_cast<unsigned char>(key.getTextCharacter())));
        const std::string keys="1234qwerasdfzxcv";const auto pos=keys.find(ch);
        if(pos!=std::string::npos){const int index=bank_*16+static_cast<int>(pos);if(index>=0&&index<static_cast<int>(s->slices.size())){auto&sl=s->slices[static_cast<std::size_t>(index)];selectedSliceId_=sl.id;preview(sl,*s,true);repaint();return true;}return false;}
        if(key!=juce::KeyPress::deleteKey&&key!=juce::KeyPress::backspaceKey)return false;if(selectedSliceId_==0)return false;
        auto it=std::find_if(s->slices.begin(),s->slices.end(),[&](auto const&sl){return sl.id==selectedSliceId_;});if(it==s->slices.end()||it==s->slices.begin())return false;const std::size_t index=static_cast<std::size_t>(std::distance(s->slices.begin(),it));
        Project before=project_;if(!removeSliceBoundary(*s,index-1))return false;selectedSliceId_=s->slices[index-1].id;if(commit_)commit_(std::move(before),"Merge sample slices");repaint();return true;
    }

private:
    SampleAsset*sample(){return project_.findSample(sampleId_);}
    const SampleAsset*sample()const{return project_.findSample(sampleId_);}
    juce::Rectangle<int>waveBounds()const{return{6,22,std::max(1,getWidth()-12),std::max(80,getHeight()-166)};}
    juce::Rectangle<int>padBounds()const{return{6,std::max(108,getHeight()-136),std::max(1,getWidth()-12),118};}
    int xForFrame(SampleIndex frame,const SampleAsset&s)const{auto r=waveBounds();const auto frames=s.audio?std::max<SampleIndex>(1,s.audio->frames()):1;return r.getX()+static_cast<int>(std::llround(static_cast<double>(std::clamp<SampleIndex>(frame,0,frames))/frames*r.getWidth()));}
    SampleIndex frameAtX(int x,const SampleAsset&s)const{auto r=waveBounds();const auto frames=s.audio?std::max<SampleIndex>(1,s.audio->frames()):1;const double u=std::clamp(static_cast<double>(x-r.getX())/std::max(1,r.getWidth()),0.0,1.0);return std::clamp<SampleIndex>(static_cast<SampleIndex>(std::llround(u*frames)),0,frames);}
    int boundaryAtX(int x,const SampleAsset&s)const{for(std::size_t i=0;i+1<s.slices.size();++i)if(std::abs(x-xForFrame(s.slices[i].endFrame,s))<=5)return static_cast<int>(i);return-1;}
    void preview(const SampleSlice&sl,SampleAsset&s,bool report=false){if(!s.audio)return;if(engine_.triggerPreview(s.audio,sl.startFrame,std::max<SampleIndex>(1,sl.endFrame-sl.startFrame),sl.gain,sl.pan,sl.chokeGroup)&&report&&trigger_)trigger_(s.id,sl.id);}
    void selectPadAt(juce::Point<int>point,SampleAsset&s){auto r=padBounds();const int col=std::clamp((point.x-r.getX())*4/std::max(1,r.getWidth()),0,3);const int row=std::clamp((point.y-r.getY())*4/std::max(1,r.getHeight()),0,3);const int index=bank_*16+row*4+col;if(index<0||index>=static_cast<int>(s.slices.size()))return;auto&sl=s.slices[static_cast<std::size_t>(index)];selectedSliceId_=sl.id;preview(sl,s,true);repaint();}

    Project&project_;AudioEngine&engine_;CommitFn commit_;TriggerFn trigger_;Id sampleId_=0,selectedSliceId_=0;int bank_=0,boundaryDrag_=-1;SampleIndex originalBoundary_=0;Project before_;
};

} // namespace flowdaw::juceui
