#pragma once
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
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void setPlayheadTick(Tick tick){playheadTick_=std::max<Tick>(0,tick);repaint();}

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff101114));
        const auto bounds=getLocalBounds();const int header=26,labelWidth=120,rowHeight=44;
        const auto timeline=juce::Rectangle<int>(labelWidth,header,std::max(1,bounds.getWidth()-labelWidth),std::max(1,bounds.getHeight()-header));
        g.setColour(juce::Colour(0xff1b1d22));g.fillRect(0,0,bounds.getWidth(),header);
        const Tick end=visibleEndTick();
        for(Tick tick=0;tick<=end;tick+=kPPQ){
            const int x=xForTick(tick,end);const bool bar=((tick/kPPQ)%4)==0;
            g.setColour(bar?juce::Colour(0xff3a3e48):juce::Colour(0xff24272e));
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
                auto r=clipRect(track.clips[ci],ti,end);g.setColour(juce::Colour(0xff6550dc));g.fillRoundedRectangle(r,4.0f);
                auto*s=project_.findSample(track.clips[ci].sampleId);g.setColour(juce::Colours::white);g.drawFittedText(s?s->name:"Audio",r.toNearestInt().reduced(5,0),juce::Justification::centredLeft,1);
            }
            for(std::size_t pi=0;pi<track.patternClips.size();++pi){
                auto r=patternRect(track.patternClips[pi],ti,end);g.setColour(juce::Colour(0xffed963c));g.fillRoundedRectangle(r,4.0f);
                auto*p=project_.findPattern(track.patternClips[pi].patternId);g.setColour(juce::Colours::white);g.drawFittedText(p?p->name:"Pattern",r.toNearestInt().reduced(5,0),juce::Justification::centredLeft,1);
            }
        }

        const int px=xForTick(std::clamp<Tick>(playheadTick_,0,end),end);g.setColour(juce::Colour(0xff30ca84));g.drawVerticalLine(px,0.0f,static_cast<float>(bounds.getBottom()));
        g.setColour(juce::Colour(0xff9aa0ad));g.setFont(11.0f);g.drawText("Drag clips/patterns • snap 1/16",8,3,labelWidth-12,18,juce::Justification::centredLeft,false);
    }

    void mouseDown(const juce::MouseEvent&e)override{
        drag_=hitTest(e.position);if(drag_.kind==Kind::None)return;
        before_=project_;anchorX_=e.x;dragStart_=currentStart();setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    void mouseDrag(const juce::MouseEvent&e)override{
        if(drag_.kind==Kind::None)return;const int content=std::max(1,getWidth()-120);const Tick end=visibleEndTick();
        const double ticksPerPixel=static_cast<double>(end)/static_cast<double>(content);
        const Tick raw=static_cast<Tick>(std::llround(static_cast<double>(e.x-anchorX_)*ticksPerPixel));
        constexpr Tick snap=kPPQ/4;const Tick snapped=static_cast<Tick>(std::llround(static_cast<double>(raw)/snap))*snap;
        setCurrentStart(std::max<Tick>(0,dragStart_+snapped));repaint();
    }
    void mouseUp(const juce::MouseEvent&)override{
        if(drag_.kind==Kind::None)return;const bool changed=currentStart()!=dragStart_;setMouseCursor(juce::MouseCursor::NormalCursor);
        auto before=std::move(before_);const auto kind=drag_.kind;drag_=Hit{};if(changed&&commit_)commit_(std::move(before),kind==Kind::Audio?"Move audio clip":"Move pattern clip");
    }

private:
    enum class Kind{None,Audio,Pattern};
    struct Hit{Kind kind=Kind::None;std::size_t track=0,index=0;};

    Tick visibleEndTick()const{
        Tick end=kPPQ*32;
        for(auto const&t:project_.tracks){
            for(auto const&c:t.clips)end=std::max(end,c.startTick+std::max<Tick>(c.lengthTicks,kPPQ));
            for(auto const&pp:t.patternClips)if(auto*p=project_.findPattern(pp.patternId))end=std::max(end,pp.startTick+p->lengthTicks()*std::max(1,pp.repeats));
        }
        const Tick bar=kPPQ*4;return ((end+bar-1)/bar)*bar;
    }
    int xForTick(Tick tick,Tick end)const{const int w=std::max(1,getWidth()-120);return 120+static_cast<int>(std::llround(static_cast<double>(tick)/std::max<Tick>(1,end)*w));}
    juce::Rectangle<float> clipRect(const Clip&c,std::size_t track,Tick end)const{
        const int y=26+static_cast<int>(track)*44;const int x=xForTick(c.startTick,end);const int x2=xForTick(c.startTick+std::max<Tick>(c.lengthTicks,kPPQ/2),end);
        return{static_cast<float>(x+2),static_cast<float>(y+6),static_cast<float>(std::max(18,x2-x-4)),32.0f};
    }
    juce::Rectangle<float> patternRect(const PatternPlacement&pp,std::size_t track,Tick end)const{
        Tick length=kPPQ;if(auto*p=project_.findPattern(pp.patternId))length=p->lengthTicks()*std::max(1,pp.repeats);
        const int y=26+static_cast<int>(track)*44;const int x=xForTick(pp.startTick,end);const int x2=xForTick(pp.startTick+length,end);
        return{static_cast<float>(x+2),static_cast<float>(y+6),static_cast<float>(std::max(18,x2-x-4)),32.0f};
    }
    Hit hitTest(juce::Point<float>point)const{
        const Tick end=visibleEndTick();
        for(std::size_t ti=0;ti<project_.tracks.size();++ti){
            auto const&t=project_.tracks[ti];
            for(std::size_t i=t.clips.size();i>0;--i)if(clipRect(t.clips[i-1],ti,end).contains(point))return{Kind::Audio,ti,i-1};
            for(std::size_t i=t.patternClips.size();i>0;--i)if(patternRect(t.patternClips[i-1],ti,end).contains(point))return{Kind::Pattern,ti,i-1};
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

    Project&project_;CommitFn commit_;Tick playheadTick_=0,dragStart_=0;int anchorX_=0;Hit drag_;Project before_;
};

} // namespace flowdaw::juceui
