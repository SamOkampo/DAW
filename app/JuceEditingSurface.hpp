#pragma once
#include "flowdaw/ArrangementSelection.hpp"
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/Project.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>

namespace flowdaw::juceui {

class StereoMeterComponent final:public juce::Component {
public:
    void setReading(const AudioMeterReading&reading){reading_=reading;repaint();}
    void clear(){reading_=AudioMeterReading{};repaint();}

    void paint(juce::Graphics&g)override{
        auto bounds=getLocalBounds().toFloat();g.setColour(juce::Colour(0xff17191e));g.fillRoundedRectangle(bounds,4.0f);
        auto inner=bounds.reduced(6.0f);auto left=inner.removeFromTop(inner.getHeight()*0.45f);inner.removeFromTop(4.0f);auto right=inner;
        drawChannel(g,left,reading_.samplePeakLeft,reading_.truePeakLeft,"L");
        drawChannel(g,right,reading_.samplePeakRight,reading_.truePeakRight,"R");
    }
private:
    static float db(float value){return value>0.000001f?20.0f*std::log10(value):-120.0f;}
    static float norm(float value){return std::clamp((db(value)+60.0f)/60.0f,0.0f,1.0f);}
    static void drawChannel(juce::Graphics&g,juce::Rectangle<float>r,float peak,float truePeak,const char*label){
        g.setColour(juce::Colour(0xff292c34));g.fillRoundedRectangle(r,3.0f);
        auto fill=r.withWidth(r.getWidth()*norm(peak));g.setColour(juce::Colour(0xff30ca84));g.fillRoundedRectangle(fill,3.0f);
        const float marker=r.getX()+r.getWidth()*norm(truePeak);g.setColour(juce::Colour(0xffed963c));g.drawVerticalLine(static_cast<int>(std::lround(marker)),r.getY(),r.getBottom());
        g.setColour(juce::Colours::white);g.setFont(11.0f);
        g.drawText(juce::String(label)+"  "+juce::String(db(peak),1)+" dB  TP "+juce::String(db(truePeak),1),r.reduced(5.0f),juce::Justification::centredLeft,false);
    }
    AudioMeterReading reading_;
};

class ArrangementComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;

    ArrangementComponent(Project&project,CommitFn commit):project_(project),commit_(std::move(commit)){
        setMouseCursor(juce::MouseCursor::NormalCursor);setWantsKeyboardFocus(true);
    }

    void setPlayheadTick(Tick tick){playheadTick_=std::max<Tick>(0,tick);repaint();}

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff101114));
        const auto bounds=getLocalBounds();const int header=26,labelWidth=120,rowHeight=44;
        const auto timeline=juce::Rectangle<int>(labelWidth,header,std::max(1,bounds.getWidth()-labelWidth),std::max(1,bounds.getHeight()-header));
        g.setColour(juce::Colour(0xff1b1d22));g.fillRect(0,0,bounds.getWidth(),header);
        const Tick end=visibleEndTick();
        const Tick grid=snapGridTicks();
        const Tick firstGrid=(viewStartTick_/grid)*grid;
        for(Tick tick=firstGrid;tick<=end;tick+=grid){
            const int x=xForTick(tick);const bool beat=(tick%kPPQ)==0;const bool bar=beat&&((tick/kPPQ)%4)==0;
            g.setColour(bar?juce::Colour(0xff3a3e48):(beat?juce::Colour(0xff24272e):juce::Colour(0xff1f2127)));
            g.drawVerticalLine(x,static_cast<float>(header),static_cast<float>(bounds.getBottom()));
            if(bar){g.setColour(juce::Colour(0xff9aa0ad));g.setFont(11.0f);g.drawText(juce::String(static_cast<int>(tick/(kPPQ*4))+1),x+3,3,40,18,juce::Justification::left,false);}
        }

        for(std::size_t ti=0;ti<project_.tracks.size();++ti){
            const int y=header+static_cast<int>(ti)*rowHeight;if(y>=bounds.getBottom())break;
            const auto row=juce::Rectangle<int>(0,y,bounds.getWidth(),rowHeight-1);
            g.setColour((ti%2)==0?juce::Colour(0xff181a1f):juce::Colour(0xff1d1f25));g.fillRect(row);
            g.setColour(juce::Colour(0xffdfe2e8));g.setFont(12.0f);g.drawFittedText(project_.tracks[ti].name,8,y,106,rowHeight,juce::Justification::centredLeft,1);
            auto const&track=project_.tracks[ti];
            for(std::size_t ci=0;ci<track.clips.size();++ci){
                auto r=clipRect(track.clips[ci],ti);g.setColour(juce::Colour(0xff6550dc));g.fillRoundedRectangle(r,4.0f);
                auto*s=project_.findSample(track.clips[ci].sampleId);g.setColour(juce::Colours::white);g.drawFittedText(s?s->name:"Audio",r.toNearestInt().reduced(5,0),juce::Justification::centredLeft,1);
                if(selection_.contains(ArrangementSelection::Kind::AudioClip,track.id,track.clips[ci].id)){g.setColour(juce::Colours::white);g.drawRoundedRectangle(r.reduced(1.0f),4.0f,1.5f);}
            }
            for(std::size_t pi=0;pi<track.patternClips.size();++pi){
                auto r=patternRect(track.patternClips[pi],ti);g.setColour(juce::Colour(0xffed963c));g.fillRoundedRectangle(r,4.0f);
                auto*p=project_.findPattern(track.patternClips[pi].patternId);g.setColour(juce::Colours::white);g.drawFittedText(juce::String(p?p->name:std::string("Pattern"))+" x"+juce::String(std::max(1,track.patternClips[pi].repeats)),r.toNearestInt().reduced(5,0),juce::Justification::centredLeft,1);
                if(selection_.contains(ArrangementSelection::Kind::PatternClip,track.id,track.patternClips[pi].id)){g.setColour(juce::Colours::white);g.drawRoundedRectangle(r.reduced(1.0f),4.0f,1.5f);}
            }
        }

        if(playheadTick_>=viewStartTick_&&playheadTick_<=end){const int px=xForTick(playheadTick_);g.setColour(juce::Colour(0xff30ca84));g.drawVerticalLine(px,0.0f,static_cast<float>(bounds.getBottom()));}
        g.setColour(juce::Colour(0xff9aa0ad));g.setFont(11.0f);g.drawFittedText("Wheel scroll • Ctrl/Cmd+wheel zoom • Alt-drag free",8,3,std::max(1,bounds.getWidth()-16),18,juce::Justification::centredLeft,1);
    }

    void mouseDown(const juce::MouseEvent&e)override{
        grabKeyboardFocus();
        const auto hit=hitTest(e.position);
        const bool toggle=e.mods.isCommandDown()||e.mods.isCtrlDown();
        if(toggle){
            drag_=Hit{};
            toggleSelectionFromHit(hit);
            syncSelectedFromPrimarySelection();
            repaint();
            return;
        }
        drag_=hit;selected_=drag_;syncSelectionFromHit(selected_);repaint();if(drag_.kind==Kind::None)return;
        before_=project_;anchorX_=e.x;dragStart_=currentStart();setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    void mouseDrag(const juce::MouseEvent&e)override{
        if(drag_.kind==Kind::None)return;const int content=std::max(1,getWidth()-120);
        const double ticksPerPixel=static_cast<double>(viewSpanTicks_)/static_cast<double>(content);
        const Tick rawDelta=static_cast<Tick>(std::llround(static_cast<double>(e.x-anchorX_)*ticksPerPixel));
        const Tick desired=std::max<Tick>(0,dragStart_+rawDelta);
        setCurrentStart(e.mods.isAltDown()?desired:snapTick(desired));repaint();
    }
    void mouseWheelMove(const juce::MouseEvent&e,const juce::MouseWheelDetails&wheel)override{
        if(e.mods.isCommandDown()||e.mods.isCtrlDown()){
            const Tick oldSpan=viewSpanTicks_;
            const Tick anchor=tickForX(e.position.x);
            const double factor=std::pow(1.8,-static_cast<double>(wheel.deltaY));
            const Tick minSpan=kPPQ*4;
            const Tick maxSpan=std::max<Tick>(minSpan,contentEndTick());
            viewSpanTicks_=std::clamp<Tick>(static_cast<Tick>(std::llround(static_cast<double>(oldSpan)*factor)),minSpan,maxSpan);
            const double anchorFraction=static_cast<double>(anchor-viewStartTick_)/static_cast<double>(std::max<Tick>(1,oldSpan));
            viewStartTick_=anchor-static_cast<Tick>(std::llround(anchorFraction*static_cast<double>(viewSpanTicks_)));
        }else{
            const float delta=std::abs(wheel.deltaX)>0.0001f?wheel.deltaX:wheel.deltaY;
            viewStartTick_-=static_cast<Tick>(std::llround(static_cast<double>(delta)*static_cast<double>(viewSpanTicks_)*0.12));
        }
        clampView();repaint();
    }
    void mouseUp(const juce::MouseEvent&)override{
        if(drag_.kind==Kind::None)return;const bool changed=currentStart()!=dragStart_;setMouseCursor(juce::MouseCursor::NormalCursor);
        auto before=std::move(before_);const auto kind=drag_.kind;drag_=Hit{};if(changed&&commit_)commit_(std::move(before),kind==Kind::Audio?"Move audio clip":"Move pattern clip");
    }

    bool keyPressed(const juce::KeyPress&key)override{
        if(selection_.empty())return false;
        if(key==juce::KeyPress::deleteKey||key==juce::KeyPress::backspaceKey){deleteSelected();return true;}
        if((key.getModifiers().isCommandDown()||key.getModifiers().isCtrlDown())&&(key.getTextCharacter()=='d'||key.getTextCharacter()=='D')){duplicateSelected();return true;}
        if(key.getTextCharacter()=='+'||key.getTextCharacter()=='='){adjustRepeats(1);return true;}
        if(key.getTextCharacter()=='-'){adjustRepeats(-1);return true;}
        return false;
    }

private:
    enum class Kind{None,Audio,Pattern};
    struct Hit{Kind kind=Kind::None;std::size_t track=0,index=0;};

    void syncSelectionFromHit(const Hit&hit){
        if(hit.kind==Kind::None||hit.track>=project_.tracks.size()){selection_.clear();return;}
        auto const&t=project_.tracks[hit.track];
        if(hit.kind==Kind::Audio&&hit.index<t.clips.size())selection_.selectAudio(t.id,t.clips[hit.index].id);
        else if(hit.kind==Kind::Pattern&&hit.index<t.patternClips.size())selection_.selectPattern(t.id,t.patternClips[hit.index].id);
        else selection_.clear();
    }
    void toggleSelectionFromHit(const Hit&hit){
        if(hit.kind==Kind::None||hit.track>=project_.tracks.size()){selection_.clear();return;}
        auto const&t=project_.tracks[hit.track];
        if(hit.kind==Kind::Audio&&hit.index<t.clips.size())selection_.toggle(ArrangementSelection::Kind::AudioClip,t.id,t.clips[hit.index].id);
        else if(hit.kind==Kind::Pattern&&hit.index<t.patternClips.size())selection_.toggle(ArrangementSelection::Kind::PatternClip,t.id,t.patternClips[hit.index].id);
    }
    void syncSelectedFromPrimarySelection(){
        selected_=Hit{};
        const auto item=selection_.item();
        if(!item.valid())return;
        for(std::size_t ti=0;ti<project_.tracks.size();++ti){
            auto const&t=project_.tracks[ti];
            if(t.id!=item.trackId)continue;
            if(item.kind==ArrangementSelection::Kind::AudioClip){
                for(std::size_t i=0;i<t.clips.size();++i)if(t.clips[i].id==item.id){selected_={Kind::Audio,ti,i};return;}
            }else if(item.kind==ArrangementSelection::Kind::PatternClip){
                for(std::size_t i=0;i<t.patternClips.size();++i)if(t.patternClips[i].id==item.id){selected_={Kind::Pattern,ti,i};return;}
            }
            return;
        }
    }

    static constexpr Tick snapGridTicks(){return kPPQ/4;}
    static Tick snapTick(Tick tick){const Tick grid=snapGridTicks();return static_cast<Tick>(std::llround(static_cast<double>(tick)/static_cast<double>(grid)))*grid;}
    Tick contentEndTick()const{
        Tick end=kPPQ*32;
        for(auto const&t:project_.tracks){
            for(auto const&c:t.clips)end=std::max(end,c.startTick+std::max<Tick>(c.lengthTicks,kPPQ));
            for(auto const&pp:t.patternClips)if(auto*p=project_.findPattern(pp.patternId))end=std::max(end,pp.startTick+p->lengthTicks()*std::max(1,pp.repeats));
        }
        const Tick bar=kPPQ*4;return ((end+bar-1)/bar)*bar;
    }
    Tick visibleEndTick()const{return viewStartTick_+viewSpanTicks_;}
    void clampView(){const Tick maxStart=std::max<Tick>(0,contentEndTick()-viewSpanTicks_);viewStartTick_=std::clamp<Tick>(viewStartTick_,0,maxStart);}
    int xForTick(Tick tick)const{const int w=std::max(1,getWidth()-120);return 120+static_cast<int>(std::llround(static_cast<double>(tick-viewStartTick_)/static_cast<double>(std::max<Tick>(1,viewSpanTicks_))*w));}
    Tick tickForX(float x)const{const int w=std::max(1,getWidth()-120);const double ratio=std::clamp((static_cast<double>(x)-120.0)/static_cast<double>(w),0.0,1.0);return viewStartTick_+static_cast<Tick>(std::llround(ratio*static_cast<double>(viewSpanTicks_)));}
    juce::Rectangle<float> clipRect(const Clip&c,std::size_t track)const{
        const int y=26+static_cast<int>(track)*44;const int x=xForTick(c.startTick);const int x2=xForTick(c.startTick+std::max<Tick>(c.lengthTicks,kPPQ/2));
        return{static_cast<float>(x+2),static_cast<float>(y+6),static_cast<float>(std::max(18,x2-x-4)),32.0f};
    }
    juce::Rectangle<float> patternRect(const PatternPlacement&pp,std::size_t track)const{
        Tick length=kPPQ;if(auto*p=project_.findPattern(pp.patternId))length=p->lengthTicks()*std::max(1,pp.repeats);
        const int y=26+static_cast<int>(track)*44;const int x=xForTick(pp.startTick);const int x2=xForTick(pp.startTick+length);
        return{static_cast<float>(x+2),static_cast<float>(y+6),static_cast<float>(std::max(18,x2-x-4)),32.0f};
    }
    Hit hitTest(juce::Point<float>point)const{
        for(std::size_t ti=0;ti<project_.tracks.size();++ti){
            auto const&t=project_.tracks[ti];
            for(std::size_t i=t.clips.size();i>0;--i)if(clipRect(t.clips[i-1],ti).contains(point))return{Kind::Audio,ti,i-1};
            for(std::size_t i=t.patternClips.size();i>0;--i)if(patternRect(t.patternClips[i-1],ti).contains(point))return{Kind::Pattern,ti,i-1};
        }
        return{};
    }
    Tick currentStart()const{
        if(drag_.track>=project_.tracks.size())return 0;auto const&t=project_.tracks[drag_.track];
        if(drag_.kind==Kind::Audio&&drag_.index<t.clips.size())return t.clips[drag_.index].startTick;
        if(drag_.kind==Kind::Pattern&&drag_.index<t.patternClips.size())return t.patternClips[drag_.index].startTick;
        return 0;
    }
    void setCurrentStart(Tick tick){
        if(drag_.track>=project_.tracks.size())return;auto&t=project_.tracks[drag_.track];
        if(drag_.kind==Kind::Audio&&drag_.index<t.clips.size())t.clips[drag_.index].startTick=tick;
        else if(drag_.kind==Kind::Pattern&&drag_.index<t.patternClips.size())t.patternClips[drag_.index].startTick=tick;
    }

    void deleteSelected(){
        if(selection_.empty())return;
        Project before=project_;
        const auto selectedCount=selection_.size();
        bool changed=false;
        for(auto&t:project_.tracks){
            const auto clipsBefore=t.clips.size();
            std::erase_if(t.clips,[&](const Clip&clip){return selection_.contains(ArrangementSelection::Kind::AudioClip,t.id,clip.id);});
            const auto patternsBefore=t.patternClips.size();
            std::erase_if(t.patternClips,[&](const PatternPlacement&placement){return selection_.contains(ArrangementSelection::Kind::PatternClip,t.id,placement.id);});
            changed=changed||clipsBefore!=t.clips.size()||patternsBefore!=t.patternClips.size();
        }
        if(!changed)return;
        drag_=Hit{};selected_=Hit{};selection_.clear();
        if(commit_)commit_(std::move(before),selectedCount>1?"Delete arrangement blocks":"Delete arrangement block");
        repaint();
    }
    void duplicateSelected(){
        if(selected_.track>=project_.tracks.size())return;Project before=project_;auto&t=project_.tracks[selected_.track];
        if(selected_.kind==Kind::Audio&&selected_.index<t.clips.size()){auto copy=t.clips[selected_.index];copy.id=nextId();copy.startTick+=std::max<Tick>(copy.lengthTicks,kPPQ/2);t.clips.push_back(copy);selected_={Kind::Audio,selected_.track,t.clips.size()-1};}
        else if(selected_.kind==Kind::Pattern&&selected_.index<t.patternClips.size()){auto copy=t.patternClips[selected_.index];copy.id=nextId();Tick len=kPPQ;if(auto*p=project_.findPattern(copy.patternId))len=p->lengthTicks()*std::max(1,copy.repeats);copy.startTick+=len;t.patternClips.push_back(copy);selected_={Kind::Pattern,selected_.track,t.patternClips.size()-1};}
        else return;syncSelectionFromHit(selected_);if(commit_)commit_(std::move(before),"Duplicate arrangement block");repaint();
    }
    void adjustRepeats(int delta){
        if(selected_.kind!=Kind::Pattern||selected_.track>=project_.tracks.size())return;auto&t=project_.tracks[selected_.track];if(selected_.index>=t.patternClips.size())return;
        const int next=std::clamp(t.patternClips[selected_.index].repeats+delta,1,64);if(next==t.patternClips[selected_.index].repeats)return;Project before=project_;t.patternClips[selected_.index].repeats=next;if(commit_)commit_(std::move(before),"Change pattern repeats");repaint();
    }

    Project&project_;CommitFn commit_;Tick playheadTick_=0,dragStart_=0,viewStartTick_=0,viewSpanTicks_=kPPQ*32;int anchorX_=0;Hit drag_,selected_;ArrangementSelection selection_;Project before_;
};

} // namespace flowdaw::juceui
