#pragma once
#include "flowdaw/Project.hpp"
#include "flowdaw/NativeDrums.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace flowdaw::juceui {

class StepSequencerComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;

    StepSequencerComponent(Project&project,CommitFn commit):project_(project),commit_(std::move(commit)){
        configureSlider(velocity_,0.0,1.5,0.01,"Velocity");
        configureSlider(probability_,0.0,1.0,0.01,"Probability");
        configureSlider(micro_, -240.0,240.0,1.0,"Micro");
        configureSlider(laneVolume_,0.0,2.0,0.01,"Lane Vol");
        configureSlider(lanePan_,-1.0,1.0,0.01,"Lane Pan");
        configureSlider(swing_,0.0,1.0,0.01,"Swing");
        configureSlider(humanize_,0.0,1.0,0.01,"Humanize");

        pagePrev_.setButtonText("Page -");pagePrev_.onClick=[this]{setPage(page_-1);};addAndMakeVisible(pagePrev_);
        pageNext_.setButtonText("Page +");pageNext_.onClick=[this]{setPage(page_+1);};addAndMakeVisible(pageNext_);
        len16_.setButtonText("16");len16_.onClick=[this]{setLength(16);};addAndMakeVisible(len16_);
        len32_.setButtonText("32");len32_.onClick=[this]{setLength(32);};addAndMakeVisible(len32_);
        len64_.setButtonText("64");len64_.onClick=[this]{setLength(64);};addAndMakeVisible(len64_);
        int drumId=1;for(auto const&choice:nativeDrums())drumChoice_.addItem(juce::String(choice.first),drumId++);drumChoice_.setTextWhenNothingSelected("Assign FLOW drum");drumChoice_.onChange=[this]{assignNativeDrum(drumChoice_.getSelectedId()-1);};addAndMakeVisible(drumChoice_);
        straight_.setButtonText("Straight");straight_.onClick=[this]{setGroovePreset(0.0f,0.0f,"Straight");};addAndMakeVisible(straight_);
        boomBap_.setButtonText("Boom Bap");boomBap_.onClick=[this]{setGroovePreset(0.18f,0.10f,"Boom Bap");};addAndMakeVisible(boomBap_);
        loose_.setButtonText("Loose");loose_.onClick=[this]{setGroovePreset(0.28f,0.22f,"Loose");};addAndMakeVisible(loose_);
    }

    void setPatternId(Id id){
        if(patternId_==id){syncControls();repaint();return;}patternId_=id;selectedLane_=0;selectedStep_=0;page_=0;gestureActive_=false;syncControls();repaint();
    }
    Id patternId()const{return patternId_;}

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff0f1115));
        auto*p=pattern();
        if(!p){g.setColour(juce::Colour(0xff9aa0ad));g.drawText("Select a drum pattern",getLocalBounds(),juce::Justification::centred);return;}

        auto grid=gridBounds();
        g.setColour(juce::Colour(0xff171a20));g.fillRoundedRectangle(grid.toFloat(),5.0f);
        const int first=page_*16;
        const int visible=std::max(0,std::min(16,p->stepCount-first));
        const int laneCount=static_cast<int>(p->lanes.size());

        for(int step=0;step<visible;++step){
            const int x=grid.getX()+kLaneHeader+(step*(grid.getWidth()-kLaneHeader))/16;
            const int x2=grid.getX()+kLaneHeader+((step+1)*(grid.getWidth()-kLaneHeader))/16;
            if(((first+step)%4)==0){g.setColour(juce::Colour(0xff252933));g.fillRect(x,grid.getY(),std::max(1,x2-x),grid.getHeight());}
            g.setColour(juce::Colour(0xff3a3e48));g.drawVerticalLine(x,static_cast<float>(grid.getY()),static_cast<float>(grid.getBottom()));
            g.setColour(juce::Colour(0xffaeb4c0));g.setFont(10.0f);g.drawText(juce::String(first+step+1),x,grid.getY()-18,std::max(1,x2-x),16,juce::Justification::centred,false);
        }

        for(int laneIndex=0;laneIndex<laneCount;++laneIndex){
            const int y=grid.getY()+laneIndex*kRowHeight;if(y>=grid.getBottom())break;
            const auto&lane=p->lanes[static_cast<std::size_t>(laneIndex)];
            const auto row=juce::Rectangle<int>(grid.getX(),y,grid.getWidth(),kRowHeight-1);
            g.setColour((laneIndex%2)==0?juce::Colour(0xff181b20):juce::Colour(0xff1d2026));g.fillRect(row);
            g.setColour(juce::Colour(0xffdfe2e8));g.setFont(11.0f);g.drawFittedText(lane.name,grid.getX()+5,y,kLaneNameWidth-8,kRowHeight,juce::Justification::centredLeft,1);
            drawToggle(g,muteRect(laneIndex),"M",lane.mute,juce::Colour(0xffd85d5d));
            drawToggle(g,soloRect(laneIndex),"S",lane.solo,juce::Colour(0xffffc06a));

            for(int local=0;local<visible;++local){
                const int stepIndex=first+local;if(stepIndex>=static_cast<int>(lane.steps.size()))continue;
                const auto cell=stepRect(laneIndex,local);const auto&ev=lane.steps[static_cast<std::size_t>(stepIndex)];
                if(ev.active){
                    const float strength=std::clamp(ev.velocity/1.5f,0.15f,1.0f);
                    g.setColour(juce::Colour::fromFloatRGBA(0.40f,0.31f,0.86f,0.35f+0.65f*strength));g.fillRoundedRectangle(cell.toFloat().reduced(2.0f),4.0f);
                    if(ev.probability<0.999f){g.setColour(juce::Colour(0xffffc06a));g.setFont(8.5f);g.drawText(juce::String(static_cast<int>(std::lround(ev.probability*100)))+"%",cell.reduced(2),juce::Justification::centred,false);}
                }else{g.setColour(juce::Colour(0xff292d35));g.fillRoundedRectangle(cell.toFloat().reduced(3.0f),3.0f);}
                if(laneIndex==selectedLane_&&stepIndex==selectedStep_){g.setColour(juce::Colours::white.withAlpha(0.9f));g.drawRoundedRectangle(cell.toFloat().reduced(1.0f),4.0f,1.5f);}
            }
        }

        g.setColour(juce::Colour(0xffaeb4c0));g.setFont(11.0f);
        const int pages=std::max(1,(p->stepCount+15)/16);
        g.drawText(juce::String(p->name)+" • page "+juce::String(page_+1)+"/"+juce::String(pages)+" • "+juce::String(p->stepCount)+" steps",6,2,getWidth()-12,18,juce::Justification::centredLeft,false);
        if(auto*ev=selectedEvent()){
            g.drawText("Step "+juce::String(selectedStep_+1)+" • vel "+juce::String(ev->velocity,2)+" • prob "+juce::String(ev->probability,2)+" • micro "+juce::String(ev->microTicks)+" ticks",6,getHeight()-18,getWidth()-12,16,juce::Justification::centredLeft,false);
        }
    }

    void resized()override{
        auto r=getLocalBounds().reduced(6);
        r.removeFromTop(20);
        auto top=r.removeFromTop(30);pagePrev_.setBounds(top.removeFromLeft(72).reduced(2));pageNext_.setBounds(top.removeFromLeft(72).reduced(2));top.removeFromLeft(8);len16_.setBounds(top.removeFromLeft(48).reduced(2));len32_.setBounds(top.removeFromLeft(48).reduced(2));len64_.setBounds(top.removeFromLeft(48).reduced(2));top.removeFromLeft(8);drumChoice_.setBounds(top.removeFromLeft(180).reduced(2));top.removeFromLeft(8);straight_.setBounds(top.removeFromLeft(82).reduced(2));boomBap_.setBounds(top.removeFromLeft(92).reduced(2));loose_.setBounds(top.removeFromLeft(72).reduced(2));
        auto controls=r.removeFromBottom(84);auto row1=controls.removeFromTop(40);layoutSlider(row1,velocity_,140);layoutSlider(row1,probability_,140);layoutSlider(row1,micro_,150);layoutSlider(row1,swing_,140);layoutSlider(row1,humanize_,140);
        auto row2=controls.removeFromTop(40);layoutSlider(row2,laneVolume_,170);layoutSlider(row2,lanePan_,170);
    }

    void mouseDown(const juce::MouseEvent&e)override{
        auto*p=pattern();if(!p)return;
        for(int laneIndex=0;laneIndex<static_cast<int>(p->lanes.size());++laneIndex){
            if(muteRect(laneIndex).contains(e.getPosition())){toggleLane(laneIndex,false);return;}
            if(soloRect(laneIndex).contains(e.getPosition())){toggleLane(laneIndex,true);return;}
            for(int local=0;local<16;++local){
                const int stepIndex=page_*16+local;if(stepIndex>=p->stepCount)break;
                if(stepRect(laneIndex,local).contains(e.getPosition())){toggleStep(laneIndex,stepIndex);return;}
            }
        }
    }

private:
    static constexpr int kRowHeight=34,kLaneHeader=118,kLaneNameWidth=70,kToggleWidth=22;

    Pattern*pattern(){return project_.findPattern(patternId_);}
    const Pattern*pattern()const{return project_.findPattern(patternId_);}
    DrumLane*selectedLane(){auto*p=pattern();return p&&selectedLane_>=0&&selectedLane_<static_cast<int>(p->lanes.size())?&p->lanes[static_cast<std::size_t>(selectedLane_)]:nullptr;}
    StepEvent*selectedEvent(){auto*l=selectedLane();return l&&selectedStep_>=0&&selectedStep_<static_cast<int>(l->steps.size())?&l->steps[static_cast<std::size_t>(selectedStep_)]:nullptr;}

    juce::Rectangle<int>gridBounds()const{
        const int top=56,bottom=98;return{6,top,std::max(1,getWidth()-12),std::max(1,getHeight()-top-bottom)};
    }
    juce::Rectangle<int>muteRect(int lane)const{auto r=gridBounds();return{r.getX()+kLaneNameWidth,r.getY()+lane*kRowHeight+6,kToggleWidth,kRowHeight-12};}
    juce::Rectangle<int>soloRect(int lane)const{auto r=gridBounds();return{r.getX()+kLaneNameWidth+kToggleWidth+2,r.getY()+lane*kRowHeight+6,kToggleWidth,kRowHeight-12};}
    juce::Rectangle<int>stepRect(int lane,int local)const{
        auto r=gridBounds();const int usable=std::max(1,r.getWidth()-kLaneHeader);const int x=r.getX()+kLaneHeader+local*usable/16;const int x2=r.getX()+kLaneHeader+(local+1)*usable/16;
        return{x,r.getY()+lane*kRowHeight,std::max(1,x2-x),kRowHeight-1};
    }
    static void drawToggle(juce::Graphics&g,juce::Rectangle<int>r,const char*label,bool on,juce::Colour onColour){
        g.setColour(on?onColour:juce::Colour(0xff30343c));g.fillRoundedRectangle(r.toFloat(),3.0f);g.setColour(juce::Colours::white);g.setFont(9.5f);g.drawText(label,r,juce::Justification::centred,false);
    }
    static void layoutSlider(juce::Rectangle<int>&row,juce::Slider&slider,int width){slider.setBounds(row.removeFromLeft(width).reduced(2));row.removeFromLeft(4);}

    static const std::array<std::pair<const char*,const char*>,7>&nativeDrums(){static const std::array<std::pair<const char*,const char*>,7> choices={{{"Kick","kick"},{"Snare","snare"},{"Closed Hat","hat"},{"Open Hat","openhat"},{"Clap","clap"},{"Rim","rim"},{"Perc","perc"}}};return choices;}
    void assignNativeDrum(int index){
        auto*p=pattern();auto*l=selectedLane();if(!p||!l||index<0||index>=static_cast<int>(nativeDrums().size()))return;const auto&choice=nativeDrums()[static_cast<std::size_t>(index)];Project before=project_;SampleAsset*asset=nullptr;for(auto&s:project_.samples)if(s.nativeKey==choice.second){asset=&s;break;}if(!asset){SampleAsset sample;sample.name=std::string("FLOW ")+choice.first;sample.nativeKey=choice.second;sample.audio=std::make_shared<AudioBuffer>(makeNativeDrum(choice.second,project_.sampleRate));project_.samples.push_back(std::move(sample));asset=&project_.samples.back();}l=selectedLane();if(!l)return;l->sampleId=asset->id;l->name=choice.first;if(commit_)commit_(std::move(before),"Assign native drum");syncControls();repaint();
    }
    void configureSlider(juce::Slider&slider,double min,double max,double step,const juce::String&suffix){
        slider.setRange(min,max,step);slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,62,22);slider.setName(suffix);addAndMakeVisible(slider);
        slider.onDragStart=[this]{beginGesture();};slider.onValueChange=[this]{applyControls();};slider.onDragEnd=[this]{endGesture();};
    }
    void setPage(int page){auto*p=pattern();if(!p)return;const int pages=std::max(1,(p->stepCount+15)/16);page_=std::clamp(page,0,pages-1);repaint();}
    void setGroovePreset(float swing,float humanize,const std::string&name){
        auto*p=pattern();if(!p)return;Project before=project_;p->swing=swing;p->humanize=humanize;if(commit_)commit_(std::move(before),name+" groove");syncControls();repaint();
    }
    void setLength(int count){
        auto*p=pattern();if(!p||p->stepCount==count)return;Project before=project_;p->stepCount=count;for(auto&lane:p->lanes)lane.steps.resize(static_cast<std::size_t>(count));selectedStep_=std::clamp(selectedStep_,0,count-1);page_=std::min(page_,(count-1)/16);if(commit_)commit_(std::move(before),"Change pattern length");syncControls();repaint();
    }
    void toggleStep(int laneIndex,int stepIndex){
        auto*p=pattern();if(!p||laneIndex<0||laneIndex>=static_cast<int>(p->lanes.size()))return;auto&lane=p->lanes[static_cast<std::size_t>(laneIndex)];if(stepIndex<0||stepIndex>=static_cast<int>(lane.steps.size()))return;
        Project before=project_;selectedLane_=laneIndex;selectedStep_=stepIndex;lane.steps[static_cast<std::size_t>(stepIndex)].active=!lane.steps[static_cast<std::size_t>(stepIndex)].active;if(commit_)commit_(std::move(before),"Toggle step");syncControls();repaint();
    }
    void toggleLane(int laneIndex,bool solo){
        auto*p=pattern();if(!p||laneIndex<0||laneIndex>=static_cast<int>(p->lanes.size()))return;Project before=project_;selectedLane_=laneIndex;auto&lane=p->lanes[static_cast<std::size_t>(laneIndex)];if(solo)lane.solo=!lane.solo;else lane.mute=!lane.mute;if(commit_)commit_(std::move(before),solo?"Toggle lane solo":"Toggle lane mute");syncControls();repaint();
    }
    void beginGesture(){if(suppress_||gestureActive_)return;before_=project_;gestureActive_=true;}
    void applyControls(){
        if(suppress_)return;auto*p=pattern();auto*l=selectedLane();auto*e=selectedEvent();if(!p)return;if(!gestureActive_)beginGesture();
        if(e){e->velocity=static_cast<float>(velocity_.getValue());e->probability=static_cast<float>(probability_.getValue());e->microTicks=static_cast<int>(std::lround(micro_.getValue()));}
        if(l){l->volume=static_cast<float>(laneVolume_.getValue());l->pan=static_cast<float>(lanePan_.getValue());}
        p->swing=static_cast<float>(swing_.getValue());p->humanize=static_cast<float>(humanize_.getValue());repaint();
    }
    void endGesture(){if(!gestureActive_)return;gestureActive_=false;if(commit_)commit_(std::move(before_),"Edit step sequencer");repaint();}
    void syncControls(){
        suppress_=true;auto*p=pattern();auto*l=selectedLane();auto*e=selectedEvent();
        velocity_.setValue(e?e->velocity:1.0,juce::dontSendNotification);probability_.setValue(e?e->probability:1.0,juce::dontSendNotification);micro_.setValue(e?e->microTicks:0,juce::dontSendNotification);
        laneVolume_.setValue(l?l->volume:1.0,juce::dontSendNotification);lanePan_.setValue(l?l->pan:0.0,juce::dontSendNotification);
        swing_.setValue(p?p->swing:0.0,juce::dontSendNotification);humanize_.setValue(p?p->humanize:0.0,juce::dontSendNotification);
        int selectedDrum=0;if(l&&l->sampleId){if(auto*smp=project_.findSample(l->sampleId)){for(std::size_t i=0;i<nativeDrums().size();++i)if(smp->nativeKey==nativeDrums()[i].second){selectedDrum=static_cast<int>(i)+1;break;}}}drumChoice_.setSelectedId(selectedDrum,juce::dontSendNotification);suppress_=false;
    }

    Project&project_;CommitFn commit_;Id patternId_=0;int page_=0,selectedLane_=0,selectedStep_=0;bool suppress_=false,gestureActive_=false;Project before_;
    juce::TextButton pagePrev_,pageNext_,len16_,len32_,len64_,straight_,boomBap_,loose_;
    juce::ComboBox drumChoice_;
    juce::Slider velocity_,probability_,micro_,laneVolume_,lanePan_,swing_,humanize_;
};

} // namespace flowdaw::juceui
