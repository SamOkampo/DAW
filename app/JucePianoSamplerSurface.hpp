#pragma once
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/Midi.hpp"
#include "flowdaw/NativeInstruments.hpp"
#include "flowdaw/Project.hpp"
#include "flowdaw/SampleEditing.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace flowdaw::juceui {

class PianoRollComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;

    PianoRollComponent(Project&project,AudioEngine&engine,CommitFn commit):project_(project),engine_(engine),commit_(std::move(commit)){
        setWantsKeyboardFocus(true);
        gridChoice_.addItem("1/8",1);gridChoice_.addItem("1/16",2);gridChoice_.addItem("1/32",3);gridChoice_.onChange=[this]{changeGrid();};addAndMakeVisible(gridChoice_);
        static const char*roots[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};for(int i=0;i<12;++i)rootChoice_.addItem(roots[i],i+1);rootChoice_.onChange=[this]{changeRoot();};addAndMakeVisible(rootChoice_);
        scaleChoice_.addItem("Major",1);scaleChoice_.addItem("Minor",2);scaleChoice_.addItem("Major Pentatonic",3);scaleChoice_.addItem("Minor Pentatonic",4);scaleChoice_.onChange=[this]{changeScale();};addAndMakeVisible(scaleChoice_);
        instrumentChoice_.addItem("FLOW Keys",1);instrumentChoice_.addItem("FLOW 808",2);instrumentChoice_.addItem("FLOW Bass",3);instrumentChoice_.addItem("FLOW Lead",4);instrumentChoice_.onChange=[this]{changeInstrument();};addAndMakeVisible(instrumentChoice_);
        octaveDown_.setButtonText("Oct -");octaveDown_.onClick=[this]{previewBasePitch_=std::max(12,previewBasePitch_-12);repaint();};addAndMakeVisible(octaveDown_);
        octaveUp_.setButtonText("Oct +");octaveUp_.onClick=[this]{previewBasePitch_=std::min(108,previewBasePitch_+12);repaint();};addAndMakeVisible(octaveUp_);
        configureSlider(velocity_,0.05,1.5,0.01," vel");configureSlider(length_,kPPQ/8,kPPQ*4,kPPQ/8," len");
        configureSlider(instGain_,0.0,2.0,0.01," gain");configureSlider(instPan_,-1.0,1.0,0.01," pan");configureSlider(instTone_,0.0,1.0,0.01," tone");
        configureSlider(instAttack_,0.0,500.0,1.0," atk");configureSlider(instRelease_,10.0,3000.0,5.0," rel");configureSlider(instDrive_,0.0,1.0,0.01," drive");configureSlider(instDelayMix_,0.0,1.0,0.01," dly");configureSlider(instDelayTicks_,0.0,kPPQ*4,kPPQ/8," dt");
    }

    void setPatternId(Id id){if(patternId_==id){syncControls();repaint();return;}patternId_=id;clearNoteSelection();dragging_=false;syncControls();repaint();}
    Id patternId()const{return patternId_;}

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff0f1115));auto*p=pattern();
        if(!p){g.setColour(juce::Colour(0xff9aa0ad));g.drawText("Select a MIDI pattern",getLocalBounds(),juce::Justification::centred);return;}
        const auto grid=gridBounds();const Tick total=visibleTicks(*p);g.setColour(juce::Colour(0xff171a20));g.fillRect(grid);
        for(int row=0;row<kRows;++row){
            const int pitch=highPitch()-row;const int y=grid.getY()+row*grid.getHeight()/kRows;const int y2=grid.getY()+(row+1)*grid.getHeight()/kRows;const bool root=(pitch%12+12)%12==p->scaleRoot;const bool inScale=pitchInScale(pitch,p->scaleRoot,p->scaleType);
            g.setColour(root?juce::Colour(0xff253033):(inScale?juce::Colour(0xff1d2424):juce::Colour(0xff181a20)));g.fillRect(grid.getX(),y,grid.getWidth(),std::max(1,y2-y-1));g.setColour(juce::Colour(0xff2a2d34));g.drawHorizontalLine(y,static_cast<float>(grid.getX()),static_cast<float>(grid.getRight()));if((pitch%12)==0){g.setColour(juce::Colour(0xffdfe2e8));g.setFont(10.0f);g.drawFittedText(midiNoteName(pitch),4,y,kKeyboardWidth-8,std::max(1,y2-y),juce::Justification::centredLeft,1);}
        }
        const Tick step=std::max<Tick>(1,p->midiGridTicks);for(Tick tick=0;tick<=total;tick+=step){const int x=xForTick(tick,total);const bool beat=(tick%kPPQ)==0;g.setColour(beat?juce::Colour(0xff414650):juce::Colour(0xff292d35));g.drawVerticalLine(x,static_cast<float>(grid.getY()),static_cast<float>(grid.getBottom()));}
        for(auto const&note:p->midiNotes){auto r=noteRect(note,*p);const bool selected=isNoteSelected(note.id);g.setColour(selected?juce::Colour(0xff8e78ff):juce::Colour(0xff6550dc));g.fillRoundedRectangle(r,3.0f);g.setColour(juce::Colours::white.withAlpha(0.9f));g.setFont(9.5f);g.drawFittedText(midiNoteName(note.pitch),r.toNearestInt().reduced(3,0),juce::Justification::centredLeft,1);if(selected){g.setColour(note.id==selectedNoteId_?juce::Colour(0xffffc06a):juce::Colours::white.withAlpha(0.65f));g.drawRoundedRectangle(r,3.0f,note.id==selectedNoteId_?1.5f:1.0f);}}
        g.setColour(juce::Colour(0xffaeb4c0));g.setFont(10.5f);g.drawText(juce::String(p->name)+" • "+juce::String(p->scaleType)+" • "+gridLabel(step)+" • preview "+midiNoteName(previewBasePitch_),kKeyboardWidth,2,std::max(1,getWidth()-kKeyboardWidth),18,juce::Justification::centredLeft,false);
        g.drawText("Ctrl/Cmd-click multi-select • Ctrl/Cmd+A all • Arrows nudge • Shift+Up/Down velocity • Delete remove",4,getHeight()-18,getWidth()-8,16,juce::Justification::centredLeft,false);
    }

    void resized()override{
        auto r=getLocalBounds().reduced(4);r.removeFromTop(20);auto selectors=r.removeFromTop(28);gridChoice_.setBounds(selectors.removeFromLeft(78).reduced(2));rootChoice_.setBounds(selectors.removeFromLeft(70).reduced(2));scaleChoice_.setBounds(selectors.removeFromLeft(145).reduced(2));instrumentChoice_.setBounds(selectors.removeFromLeft(125).reduced(2));octaveDown_.setBounds(selectors.removeFromLeft(62).reduced(2));octaveUp_.setBounds(selectors.removeFromLeft(62).reduced(2));
        auto row1=r.removeFromTop(28);layoutSlider(row1,velocity_,118);layoutSlider(row1,length_,130);layoutSlider(row1,instGain_,118);layoutSlider(row1,instPan_,118);layoutSlider(row1,instTone_,118);
        auto row2=r.removeFromTop(28);layoutSlider(row2,instAttack_,118);layoutSlider(row2,instRelease_,128);layoutSlider(row2,instDrive_,118);layoutSlider(row2,instDelayMix_,118);layoutSlider(row2,instDelayTicks_,130);
    }

    void mouseDown(const juce::MouseEvent&e)override{
        grabKeyboardFocus();auto*p=pattern();if(!p||!gridBounds().contains(e.getPosition()))return;const Id hit=noteAt(e.position,*p);
        if(hit!=0&&e.mods.isRightButtonDown()){if(!isNoteSelected(hit))selectOnlyNote(hit);deleteSelected();return;}
        if(hit==0){Project before=project_;MidiNote note;note.startTick=tickAtX(e.x,*p);note.lengthTicks=std::max<Tick>(1,p->midiDefaultLengthTicks);note.pitch=pitchAtY(e.y);note.velocity=0.9f;p->midiNotes.push_back(note);selectOnlyNote(note.id);if(commit_)commit_(std::move(before),"Add MIDI note");syncControls();previewPitch(note.pitch);repaint();return;}
        if(e.mods.isCommandDown()||e.mods.isCtrlDown()){toggleNoteSelection(hit);dragging_=false;syncControls();repaint();return;}
        selectOnlyNote(hit);auto*note=selectedNote();if(!note)return;before_=project_;dragging_=true;dragStartX_=e.x;dragStartY_=e.y;originalStart_=note->startTick;originalLength_=note->lengthTicks;originalPitch_=note->pitch;const auto nr=noteRect(*note,*p);resizing_=e.position.x>=nr.getRight()-7.0f;syncControls();repaint();
    }

    void mouseDrag(const juce::MouseEvent&e)override{
        if(!dragging_)return;auto*p=pattern();auto*note=selectedNote();if(!p||!note)return;const Tick total=visibleTicks(*p);const int width=std::max(1,gridBounds().getWidth());const Tick deltaRaw=static_cast<Tick>(std::llround(static_cast<double>(e.x-dragStartX_)*static_cast<double>(total)/width));const Tick grid=std::max<Tick>(1,p->midiGridTicks);const Tick delta=static_cast<Tick>(std::llround(static_cast<double>(deltaRaw)/grid))*grid;
        if(resizing_)note->lengthTicks=std::max<Tick>(grid/2,originalLength_+delta);else{note->startTick=snapMidiTick(std::max<Tick>(0,originalStart_+delta),grid);const int rowHeight=std::max(1,gridBounds().getHeight()/kRows);const int pitchDelta=-(e.y-dragStartY_)/rowHeight;note->pitch=std::clamp(originalPitch_+pitchDelta,0,127);}syncControls();repaint();
    }

    void mouseUp(const juce::MouseEvent&)override{
        if(!dragging_)return;dragging_=false;auto*note=selectedNote();bool changed=false;if(note)changed=note->startTick!=originalStart_||note->lengthTicks!=originalLength_||note->pitch!=originalPitch_;if(changed&&commit_)commit_(std::move(before_),resizing_?"Resize MIDI note":"Move MIDI note");resizing_=false;syncControls();repaint();
    }

    bool keyPressed(const juce::KeyPress&key)override{
        auto*p=pattern();if(!p)return false;const auto mods=key.getModifiers();
        if(mods.isCommandDown()&&key.getKeyCode()=='A'){selectAllNotes();return true;}
        if(key==juce::KeyPress::deleteKey||key==juce::KeyPress::backspaceKey)return deleteSelected();
        if(key.getKeyCode()==juce::KeyPress::leftKey)return nudgeSelected(-std::max<Tick>(1,p->midiGridTicks),0);
        if(key.getKeyCode()==juce::KeyPress::rightKey)return nudgeSelected(std::max<Tick>(1,p->midiGridTicks),0);
        if(key.getKeyCode()==juce::KeyPress::upKey)return mods.isShiftDown()?adjustSelectedNote(0.05f,0):nudgeSelected(0,1);
        if(key.getKeyCode()==juce::KeyPress::downKey)return mods.isShiftDown()?adjustSelectedNote(-0.05f,0):nudgeSelected(0,-1);
        const auto ch=static_cast<char>(std::tolower(static_cast<unsigned char>(key.getTextCharacter())));if(ch=='['){previewBasePitch_=std::max(12,previewBasePitch_-12);repaint();return true;}if(ch==']'){previewBasePitch_=std::min(108,previewBasePitch_+12);repaint();return true;}
        const std::string keys="awsedftgyhuj";const auto pos=keys.find(ch);if(pos!=std::string::npos){static const int semis[]={0,1,2,3,4,5,6,7,8,9,10,11};previewPitch(std::clamp(previewBasePitch_+semis[pos],0,127));return true;}return false;
    }

private:
    static constexpr int kKeyboardWidth=64,kHeader=104,kFooter=20,kRows=48;
    Pattern*pattern(){return project_.findPattern(patternId_);}const Pattern*pattern()const{return project_.findPattern(patternId_);}
    bool isNoteSelected(Id id)const{for(std::size_t i=0;i<selectedNoteCount_;++i)if(selectedNoteIds_[i]==id)return true;return false;}
    void clearNoteSelection(){selectedNoteCount_=0;selectedNoteId_=0;}
    void selectOnlyNote(Id id){selectedNoteCount_=id?1:0;selectedNoteId_=id;if(id)selectedNoteIds_[0]=id;}
    void toggleNoteSelection(Id id){
        for(std::size_t i=0;i<selectedNoteCount_;++i)if(selectedNoteIds_[i]==id){for(std::size_t j=i+1;j<selectedNoteCount_;++j)selectedNoteIds_[j-1]=selectedNoteIds_[j];--selectedNoteCount_;selectedNoteId_=selectedNoteCount_?selectedNoteIds_[selectedNoteCount_-1]:0;return;}
        if(selectedNoteCount_<selectedNoteIds_.size()){selectedNoteIds_[selectedNoteCount_++]=id;selectedNoteId_=id;}
    }
    void selectAllNotes(){auto*p=pattern();if(!p)return;selectedNoteCount_=std::min<std::size_t>(selectedNoteIds_.size(),p->midiNotes.size());for(std::size_t i=0;i<selectedNoteCount_;++i)selectedNoteIds_[i]=p->midiNotes[i].id;selectedNoteId_=selectedNoteCount_?selectedNoteIds_[selectedNoteCount_-1]:0;syncControls();repaint();}
    MidiNote*selectedNote(){auto*p=pattern();if(!p)return nullptr;for(auto&n:p->midiNotes)if(n.id==selectedNoteId_)return &n;return nullptr;}
    const MidiNote*selectedNote()const{auto*p=pattern();if(!p)return nullptr;for(auto const&n:p->midiNotes)if(n.id==selectedNoteId_)return &n;return nullptr;}
    int lowPitch()const{return std::clamp(previewBasePitch_-24,0,127-kRows+1);}int highPitch()const{return lowPitch()+kRows-1;}
    juce::Rectangle<int>gridBounds()const{return{kKeyboardWidth,kHeader,std::max(1,getWidth()-kKeyboardWidth),std::max(1,getHeight()-kHeader-kFooter)};}
    static Tick visibleTicks(const Pattern&p){return std::max<Tick>(p.lengthTicks(),kPPQ*4);}
    int xForTick(Tick tick,Tick total)const{auto r=gridBounds();return r.getX()+static_cast<int>(std::llround(static_cast<double>(tick)/std::max<Tick>(1,total)*r.getWidth()));}
    Tick tickAtX(int x,const Pattern&p)const{auto r=gridBounds();const double u=std::clamp(static_cast<double>(x-r.getX())/std::max(1,r.getWidth()),0.0,1.0);return snapMidiTick(static_cast<Tick>(std::llround(u*visibleTicks(p))),std::max<Tick>(1,p.midiGridTicks));}
    int pitchAtY(int y)const{auto r=gridBounds();const double u=std::clamp(static_cast<double>(y-r.getY())/std::max(1,r.getHeight()),0.0,0.999999);return std::clamp(highPitch()-static_cast<int>(u*kRows),0,127);}
    juce::Rectangle<float>noteRect(const MidiNote&n,const Pattern&p)const{auto r=gridBounds();const Tick total=visibleTicks(p);const int row=highPitch()-std::clamp(n.pitch,lowPitch(),highPitch());const int y=r.getY()+row*r.getHeight()/kRows;const int y2=r.getY()+(row+1)*r.getHeight()/kRows;const int x=xForTick(n.startTick,total);const int x2=xForTick(n.startTick+std::max<Tick>(1,n.lengthTicks),total);return{static_cast<float>(x+1),static_cast<float>(y+1),static_cast<float>(std::max(5,x2-x-2)),static_cast<float>(std::max(4,y2-y-2))};}
    Id noteAt(juce::Point<float>point,const Pattern&p)const{for(auto it=p.midiNotes.rbegin();it!=p.midiNotes.rend();++it)if(it->pitch>=lowPitch()&&it->pitch<=highPitch()&&noteRect(*it,p).contains(point))return it->id;return 0;}
    static juce::String gridLabel(Tick grid){if(grid==kPPQ/2)return"1/8";if(grid==kPPQ/4)return"1/16";if(grid==kPPQ/8)return"1/32";return juce::String(static_cast<int>(grid));}
    static void layoutSlider(juce::Rectangle<int>&row,juce::Slider&slider,int width){slider.setBounds(row.removeFromLeft(width).reduced(1));row.removeFromLeft(3);}
    void configureSlider(juce::Slider&slider,double min,double max,double step,const juce::String&suffix){slider.setRange(min,max,step);slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,66,20);slider.setTextValueSuffix(suffix);slider.onDragStart=[this]{beginControlGesture();};slider.onValueChange=[this]{applyControlValues();};slider.onDragEnd=[this]{endControlGesture();};addAndMakeVisible(slider);}
    void beginControlGesture(){if(suppressControls_||controlGesture_)return;controlBefore_=project_;controlGesture_=true;}
    void applyControlValues(){if(suppressControls_)return;auto*p=pattern();if(!p)return;if(!controlGesture_)beginControlGesture();for(auto&n:p->midiNotes)if(isNoteSelected(n.id)){n.velocity=static_cast<float>(velocity_.getValue());n.lengthTicks=std::max<Tick>(1,static_cast<Tick>(std::llround(length_.getValue())));}auto&i=p->instrument;i.gain=static_cast<float>(instGain_.getValue());i.pan=static_cast<float>(instPan_.getValue());i.tone=static_cast<float>(instTone_.getValue());i.attackMs=static_cast<float>(instAttack_.getValue());i.releaseMs=static_cast<float>(instRelease_.getValue());i.drive=static_cast<float>(instDrive_.getValue());i.delayMix=static_cast<float>(instDelayMix_.getValue());i.delayTicks=static_cast<Tick>(std::llround(instDelayTicks_.getValue()));repaint();}
    void endControlGesture(){if(!controlGesture_)return;controlGesture_=false;if(commit_)commit_(std::move(controlBefore_),"Edit Piano Roll controls");syncControls();repaint();}
    void syncControls(){suppressControls_=true;auto*p=pattern();auto*n=selectedNote();velocity_.setValue(n?n->velocity:0.9,juce::dontSendNotification);length_.setValue(n?n->lengthTicks:(p?p->midiDefaultLengthTicks:kPPQ/2),juce::dontSendNotification);if(p){gridChoice_.setSelectedId(p->midiGridTicks==kPPQ/2?1:(p->midiGridTicks==kPPQ/4?2:3),juce::dontSendNotification);rootChoice_.setSelectedId(std::clamp(p->scaleRoot,0,11)+1,juce::dontSendNotification);int scale=2;if(p->scaleType=="major")scale=1;else if(p->scaleType=="major_pentatonic")scale=3;else if(p->scaleType=="minor_pentatonic")scale=4;scaleChoice_.setSelectedId(scale,juce::dontSendNotification);int inst=1;if(p->instrument.type=="flow_808")inst=2;else if(p->instrument.type=="flow_bass")inst=3;else if(p->instrument.type=="flow_lead")inst=4;instrumentChoice_.setSelectedId(inst,juce::dontSendNotification);const auto&i=p->instrument;instGain_.setValue(i.gain,juce::dontSendNotification);instPan_.setValue(i.pan,juce::dontSendNotification);instTone_.setValue(i.tone,juce::dontSendNotification);instAttack_.setValue(i.attackMs,juce::dontSendNotification);instRelease_.setValue(i.releaseMs,juce::dontSendNotification);instDrive_.setValue(i.drive,juce::dontSendNotification);instDelayMix_.setValue(i.delayMix,juce::dontSendNotification);instDelayTicks_.setValue(i.delayTicks,juce::dontSendNotification);}suppressControls_=false;}
    void changeGrid(){if(suppressControls_)return;auto*p=pattern();if(!p)return;Project before=project_;const int id=gridChoice_.getSelectedId();p->midiGridTicks=id==1?kPPQ/2:(id==2?kPPQ/4:kPPQ/8);if(commit_)commit_(std::move(before),"Change MIDI grid");repaint();}
    void changeRoot(){if(suppressControls_)return;auto*p=pattern();if(!p||rootChoice_.getSelectedId()<=0)return;Project before=project_;p->scaleRoot=rootChoice_.getSelectedId()-1;if(commit_)commit_(std::move(before),"Change scale root");repaint();}
    void changeScale(){if(suppressControls_)return;auto*p=pattern();if(!p)return;static const char*types[]={"major","minor","major_pentatonic","minor_pentatonic"};const int index=std::clamp(scaleChoice_.getSelectedId()-1,0,3);Project before=project_;p->scaleType=types[index];if(commit_)commit_(std::move(before),"Change scale");repaint();}
    void changeInstrument(){if(suppressControls_)return;auto*p=pattern();if(!p)return;static const char*types[]={"flow_keys","flow_808","flow_bass","flow_lead"};const int index=std::clamp(instrumentChoice_.getSelectedId()-1,0,3);Project before=project_;p->instrument.enabled=true;p->instrument.type=types[index];if(commit_)commit_(std::move(before),"Change native instrument");syncControls();repaint();}
    bool deleteSelected(){auto*p=pattern();if(!p||selectedNoteCount_==0)return false;Project before=project_;const auto oldSize=p->midiNotes.size();std::erase_if(p->midiNotes,[this](auto const&n){return isNoteSelected(n.id);});if(p->midiNotes.size()==oldSize)return false;const auto count=selectedNoteCount_;clearNoteSelection();if(commit_)commit_(std::move(before),count>1?"Delete MIDI notes":"Delete MIDI note");syncControls();repaint();return true;}
    bool adjustSelectedNote(float velocityDelta,Tick lengthDelta){auto*p=pattern();if(!p||selectedNoteCount_==0)return false;Project before=project_;bool changed=false;for(auto&n:p->midiNotes)if(isNoteSelected(n.id)){n.velocity=std::clamp(n.velocity+velocityDelta,0.05f,1.5f);n.lengthTicks=std::max<Tick>(p->midiGridTicks/2,n.lengthTicks+lengthDelta);changed=true;}if(!changed)return false;if(commit_)commit_(std::move(before),selectedNoteCount_>1?"Edit MIDI notes":"Edit MIDI note");syncControls();repaint();return true;}
    bool nudgeSelected(Tick dt,int dp){auto*p=pattern();if(!p||selectedNoteCount_==0)return false;Project before=project_;bool changed=false;int pitch=previewBasePitch_;for(auto&n:p->midiNotes)if(isNoteSelected(n.id)){n.startTick=snapMidiTick(std::max<Tick>(0,n.startTick+dt),std::max<Tick>(1,p->midiGridTicks));n.pitch=std::clamp(n.pitch+dp,0,127);if(n.id==selectedNoteId_)pitch=n.pitch;changed=true;}if(!changed)return false;if(commit_)commit_(std::move(before),selectedNoteCount_>1?"Nudge MIDI notes":"Nudge MIDI note");previewPitch(pitch);syncControls();repaint();return true;}
    void previewPitch(int pitch){auto*p=pattern();if(!p)return;InstrumentState state=p->instrument;if(!state.enabled){state.enabled=true;state.type="flow_keys";}const auto frames=std::max<SampleIndex>(256,static_cast<SampleIndex>(engine_.sampleRate()*0.35));auto audio=std::make_shared<AudioBuffer>(renderNativeInstrumentNote(state,pitch,0.9f,frames,engine_.sampleRate(),project_.transport.bpm));engine_.triggerPreview(audio,0,audio->frames(),std::clamp(state.gain,0.0f,2.0f),std::clamp(state.pan,-1.0f,1.0f),0);}

    Project&project_;AudioEngine&engine_;CommitFn commit_;Id patternId_=0,selectedNoteId_=0;std::array<Id,256>selectedNoteIds_{};std::size_t selectedNoteCount_=0;bool dragging_=false,resizing_=false,suppressControls_=false,controlGesture_=false;int dragStartX_=0,dragStartY_=0;Tick originalStart_=0,originalLength_=0;int originalPitch_=60,previewBasePitch_=60;Project before_,controlBefore_;
    juce::ComboBox gridChoice_,rootChoice_,scaleChoice_,instrumentChoice_;juce::TextButton octaveDown_,octaveUp_;
    juce::Slider velocity_,length_,instGain_,instPan_,instTone_,instAttack_,instRelease_,instDrive_,instDelayMix_,instDelayTicks_;
};

class SamplerComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;
    using TriggerFn=std::function<void(Id,Id)>;

    SamplerComponent(Project&project,AudioEngine&engine,CommitFn commit,TriggerFn trigger={}):project_(project),engine_(engine),commit_(std::move(commit)),trigger_(std::move(trigger)){
        setWantsKeyboardFocus(true);
    }

    void setSampleId(Id id){if(sampleId_==id){repaint();return;}sampleId_=id;selectedSliceId_=0;boundaryDrag_=-1;bank_=0;repaint();}
    Id sampleId()const{return sampleId_;}
    void previousBank(){bank_=std::max(0,bank_-1);repaint();}
    void nextBank(){auto*s=sample();if(!s)return;const int banks=std::max(1,(static_cast<int>(s->slices.size())+15)/16);bank_=std::min(banks-1,bank_+1);repaint();}
    int bank()const{return bank_;}
    juce::String selectedPadName()const{auto*s=sample();if(!s||selectedSliceId_==0)return{};for(auto const&sl:s->slices)if(sl.id==selectedSliceId_)return juce::String(sl.name);return{};}
    bool renameSelectedPad(const std::string&name){auto*s=sample();if(!s||selectedSliceId_==0)return false;auto it=std::find_if(s->slices.begin(),s->slices.end(),[&](auto const&sl){return sl.id==selectedSliceId_;});if(it==s->slices.end())return false;Project before=project_;it->name=name.empty()?"Slice":name;if(commit_)commit_(std::move(before),"Rename sample pad");repaint();return true;}
    bool adjustSelectedPad(float gainDelta,float panDelta,int chokeDelta){auto*s=sample();if(!s||selectedSliceId_==0)return false;auto it=std::find_if(s->slices.begin(),s->slices.end(),[&](auto const&sl){return sl.id==selectedSliceId_;});if(it==s->slices.end())return false;Project before=project_;it->gain=std::clamp(it->gain+gainDelta,0.0f,2.0f);it->pan=std::clamp(it->pan+panDelta,-1.0f,1.0f);it->chokeGroup=std::clamp(it->chokeGroup+chokeDelta,0,8);if(commit_)commit_(std::move(before),"Edit sample pad");repaint();return true;}

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
