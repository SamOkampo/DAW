#pragma once
#include "flowdaw/Automation.hpp"
#include "flowdaw/ProductionAssistant.hpp"
#include "flowdaw/Project.hpp"
#include "flowdaw/Workflow.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <utility>

namespace flowdaw::juceui {

class AutomationAssistComponent final:public juce::Component {
public:
    using CommitFn=std::function<void(Project,std::string)>;
    using PlayheadFn=std::function<Tick()>;

    AutomationAssistComponent(Project&project,PlayheadFn playhead,CommitFn commit)
        :project_(project),playhead_(std::move(playhead)),commit_(std::move(commit)){
        targetChoice_.addItem("Master Volume",1);targetChoice_.addItem("Track Volume",2);targetChoice_.addItem("Track Pan",3);targetChoice_.addItem("Bus Volume",4);targetChoice_.addItem("Bus Pan",5);
        targetChoice_.setSelectedId(1,juce::dontSendNotification);targetChoice_.onChange=[this]{refreshRouteChoice();syncAutomationValue();repaint();};addAndMakeVisible(targetChoice_);
        routeChoice_.onChange=[this]{syncAutomationValue();repaint();};addAndMakeVisible(routeChoice_);

        value_.setSliderStyle(juce::Slider::LinearHorizontal);value_.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,22);addAndMakeVisible(value_);
        writePoint_.setButtonText("Write Point");writePoint_.onClick=[this]{writeAutomationPoint();};addAndMakeVisible(writePoint_);
        clearLane_.setButtonText("Clear Lane");clearLane_.onClick=[this]{clearAutomationLane();};addAndMakeVisible(clearLane_);

        addAndMakeVisible(busChoice_);busChoice_.onChange=[this]{syncBusControls();};
        busVolume_.setRange(0.0,2.0,0.01);busVolume_.setSliderStyle(juce::Slider::LinearHorizontal);busVolume_.setTextBoxStyle(juce::Slider::TextBoxRight,false,62,22);busVolume_.onDragStart=[this]{beginBusGesture();};busVolume_.onValueChange=[this]{applyBusControls();};busVolume_.onDragEnd=[this]{endBusGesture();};addAndMakeVisible(busVolume_);
        busPan_.setRange(-1.0,1.0,0.01);busPan_.setSliderStyle(juce::Slider::LinearHorizontal);busPan_.setTextBoxStyle(juce::Slider::TextBoxRight,false,62,22);busPan_.onDragStart=[this]{beginBusGesture();};busPan_.onValueChange=[this]{applyBusControls();};busPan_.onDragEnd=[this]{endBusGesture();};addAndMakeVisible(busPan_);
        busMute_.setButtonText("Mute");busMute_.onClick=[this]{toggleBus(false);};addAndMakeVisible(busMute_);
        busSolo_.setButtonText("Solo");busSolo_.onClick=[this]{toggleBus(true);};addAndMakeVisible(busSolo_);
        createBus_.setButtonText("Create Bus");createBus_.onClick=[this]{runWorkflow("create-mix-bus");};addAndMakeVisible(createBus_);

        addAndMakeVisible(suggestionChoice_);suggestionChoice_.onChange=[this]{syncSuggestionReport();};
        refreshAssist_.setButtonText("Analyze");refreshAssist_.onClick=[this]{refreshAssist();};addAndMakeVisible(refreshAssist_);
        applySuggestion_.setButtonText("Apply");applySuggestion_.onClick=[this]{applySuggestion();};addAndMakeVisible(applySuggestion_);

        addAndMakeVisible(commandChoice_);executeCommand_.setButtonText("Run Command");executeCommand_.onClick=[this]{runSelectedCommand();};addAndMakeVisible(executeCommand_);

        report_.setMultiLine(true);report_.setReadOnly(true);report_.setScrollbarsShown(true);report_.setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff15181d));report_.setColour(juce::TextEditor::textColourId,juce::Colour(0xffdfe2e8));addAndMakeVisible(report_);
        refresh();
    }

    void refresh(){
        const int busPrevious=busChoice_.getSelectedId(),routePrevious=routeChoice_.getSelectedId();
        busChoice_.clear(juce::dontSendNotification);int id=1;for(auto const&bus:project_.buses)busChoice_.addItem(juce::String(bus.name),id++);
        if(!project_.buses.empty())busChoice_.setSelectedId(busPrevious>0&&busPrevious<=static_cast<int>(project_.buses.size())?busPrevious:1,juce::dontSendNotification);
        refreshRouteChoice(routePrevious);syncBusControls();syncAutomationValue();refreshAssist();refreshCommands();repaint();
    }

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff0f1115));
        g.setColour(juce::Colour(0xffdfe2e8));g.setFont(12.0f);
        g.drawText("Automation",6,2,360,18,juce::Justification::centredLeft,false);
        g.drawText("Bus Mixer",382,2,320,18,juce::Justification::centredLeft,false);
        g.drawText("Assist / Project Health",716,2,std::max(1,getWidth()-722),18,juce::Justification::centredLeft,false);

        auto graph=automationGraph();
        g.setColour(juce::Colour(0xff171a20));g.fillRoundedRectangle(graph.toFloat(),5.0f);
        g.setColour(juce::Colour(0xff343944));for(int i=1;i<4;++i)g.drawVerticalLine(graph.getX()+i*graph.getWidth()/4,static_cast<float>(graph.getY()),static_cast<float>(graph.getBottom()));
        if(auto*lane=currentLane();lane&&!lane->points.empty()){
            Tick maxTick=std::max<Tick>(kPPQ*4,lane->points.back().tick);juce::Path path;bool first=true;
            for(auto const&pt:lane->points){const float x=graph.getX()+static_cast<float>(pt.tick)/maxTick*graph.getWidth();const float norm=normalizeValue(pt.value);const float y=graph.getBottom()-norm*graph.getHeight();if(first){path.startNewSubPath(x,y);first=false;}else path.lineTo(x,y);g.setColour(juce::Colour(0xffffc06a));g.fillEllipse(x-3,y-3,6,6);}
            g.setColour(juce::Colour(0xff8e78ff));g.strokePath(path,juce::PathStrokeType(2.0f));
        }
    }

    void resized()override{
        const int top=24;auto left=juce::Rectangle<int>(6,top,360,std::max(1,getHeight()-top-6));auto mid=juce::Rectangle<int>(382,top,320,std::max(1,getHeight()-top-6));auto right=juce::Rectangle<int>(716,top,std::max(1,getWidth()-722),std::max(1,getHeight()-top-6));
        targetChoice_.setBounds(left.removeFromTop(28));routeChoice_.setBounds(left.removeFromTop(28));value_.setBounds(left.removeFromTop(32));auto buttons=left.removeFromTop(32);writePoint_.setBounds(buttons.removeFromLeft(120).reduced(2));clearLane_.setBounds(buttons.removeFromLeft(110).reduced(2));

        busChoice_.setBounds(mid.removeFromTop(28));busVolume_.setBounds(mid.removeFromTop(32));busPan_.setBounds(mid.removeFromTop(32));auto busButtons=mid.removeFromTop(32);busMute_.setBounds(busButtons.removeFromLeft(70).reduced(2));busSolo_.setBounds(busButtons.removeFromLeft(70).reduced(2));createBus_.setBounds(busButtons.removeFromLeft(105).reduced(2));

        suggestionChoice_.setBounds(right.removeFromTop(28));auto assistButtons=right.removeFromTop(32);refreshAssist_.setBounds(assistButtons.removeFromLeft(90).reduced(2));applySuggestion_.setBounds(assistButtons.removeFromLeft(90).reduced(2));commandChoice_.setBounds(right.removeFromTop(28));executeCommand_.setBounds(right.removeFromTop(30).removeFromLeft(130).reduced(2));right.removeFromTop(4);report_.setBounds(right);
    }

private:
    juce::Rectangle<int>automationGraph()const{return{6,std::max(154,getHeight()-94),360,88};}
    int targetKind()const{return targetChoice_.getSelectedId();}
    bool targetNeedsRoute()const{return targetKind()!=1;}
    bool targetIsBus()const{return targetKind()==4||targetKind()==5;}
    bool targetIsPan()const{return targetKind()==3||targetKind()==5;}
    std::string targetName()const{switch(targetKind()){case 2:return"track.volume";case 3:return"track.pan";case 4:return"bus.volume";case 5:return"bus.pan";default:return"master.volume";}}
    Id targetId()const{
        if(targetKind()==1)return 0;const int index=routeChoice_.getSelectedId()-1;
        if(targetIsBus())return index>=0&&index<static_cast<int>(project_.buses.size())?project_.buses[static_cast<std::size_t>(index)].id:0;
        return index>=0&&index<static_cast<int>(project_.tracks.size())?project_.tracks[static_cast<std::size_t>(index)].id:0;
    }
    const AutomationLane*currentLane()const{return findAutomation(project_,targetName(),targetId());}
    AutomationLane*currentLaneMutable(){
        const auto name=targetName();const Id id=targetId();for(auto&lane:project_.automation)if(lane.target==name&&lane.targetId==id&&lane.subTargetId==0)return &lane;return nullptr;
    }
    float fallbackValue()const{
        if(targetKind()==1)return project_.master.volume;const int index=routeChoice_.getSelectedId()-1;
        if(targetIsBus()){if(index<0||index>=static_cast<int>(project_.buses.size()))return targetIsPan()?0.0f:1.0f;auto const&b=project_.buses[static_cast<std::size_t>(index)];return targetIsPan()?b.mixer.pan:b.mixer.volume;}
        if(index<0||index>=static_cast<int>(project_.tracks.size()))return targetIsPan()?0.0f:1.0f;auto const&t=project_.tracks[static_cast<std::size_t>(index)];return targetIsPan()?t.mixer.pan:t.mixer.volume;
    }
    float normalizeValue(float value)const{return targetIsPan()?std::clamp((value+1.0f)*0.5f,0.0f,1.0f):std::clamp(value*0.5f,0.0f,1.0f);}

    void refreshRouteChoice(int preferred=0){
        routeChoice_.clear(juce::dontSendNotification);routeChoice_.setEnabled(targetNeedsRoute());
        if(!targetNeedsRoute()){routeChoice_.setTextWhenNothingSelected("Master");return;}
        int id=1;if(targetIsBus())for(auto const&b:project_.buses)routeChoice_.addItem(juce::String(b.name),id++);else for(auto const&t:project_.tracks)routeChoice_.addItem(juce::String(t.name),id++);
        const int count=id-1;if(count>0)routeChoice_.setSelectedId(preferred>0&&preferred<=count?preferred:1,juce::dontSendNotification);
    }
    void syncAutomationValue(){
        suppress_=true;value_.setRange(targetIsPan()?-1.0:0.0,targetIsPan()?1.0:2.0,0.01);value_.setValue(automationValueAtLane(),juce::dontSendNotification);suppress_=false;repaint();
    }
    float automationValueAtLane()const{auto*lane=currentLane();const Tick tick=playhead_?playhead_():0;return lane?automationValueAt(*lane,tick,fallbackValue()):fallbackValue();}

    void writeAutomationPoint(){
        if(targetNeedsRoute()&&targetId()==0)return;Project before=project_;auto*lane=currentLaneMutable();if(!lane){AutomationLane created;created.target=targetName();created.targetId=targetId();project_.automation.push_back(created);lane=&project_.automation.back();}
        lane->points.push_back({std::max<Tick>(0,playhead_?playhead_():0),static_cast<float>(value_.getValue())});normalizeAutomationLane(*lane);if(commit_)commit_(std::move(before),"Write automation point");repaint();
    }
    void clearAutomationLane(){
        const auto name=targetName();const Id id=targetId();auto it=std::find_if(project_.automation.begin(),project_.automation.end(),[&](auto const&lane){return lane.target==name&&lane.targetId==id&&lane.subTargetId==0;});if(it==project_.automation.end())return;Project before=project_;project_.automation.erase(it);if(commit_)commit_(std::move(before),"Clear automation lane");syncAutomationValue();repaint();
    }

    int selectedBusIndex()const{const int index=busChoice_.getSelectedId()-1;return index>=0&&index<static_cast<int>(project_.buses.size())?index:-1;}
    void syncBusControls(){suppress_=true;const int index=selectedBusIndex();if(index<0){busVolume_.setValue(1.0,juce::dontSendNotification);busPan_.setValue(0.0,juce::dontSendNotification);busMute_.setButtonText("Mute");busSolo_.setButtonText("Solo");}else{auto const&b=project_.buses[static_cast<std::size_t>(index)];busVolume_.setValue(b.mixer.volume,juce::dontSendNotification);busPan_.setValue(b.mixer.pan,juce::dontSendNotification);busMute_.setButtonText(b.mixer.mute?"Muted":"Mute");busSolo_.setButtonText(b.mixer.solo?"Soloed":"Solo");}suppress_=false;}
    void beginBusGesture(){if(suppress_||busGesture_)return;busBefore_=project_;busGesture_=true;}
    void applyBusControls(){if(suppress_)return;const int index=selectedBusIndex();if(index<0)return;if(!busGesture_)beginBusGesture();auto&b=project_.buses[static_cast<std::size_t>(index)];b.mixer.volume=static_cast<float>(busVolume_.getValue());b.mixer.pan=static_cast<float>(busPan_.getValue());}
    void endBusGesture(){if(!busGesture_)return;busGesture_=false;if(commit_)commit_(std::move(busBefore_),"Edit bus mixer");}
    void toggleBus(bool solo){const int index=selectedBusIndex();if(index<0)return;Project before=project_;auto&b=project_.buses[static_cast<std::size_t>(index)];if(solo)b.mixer.solo=!b.mixer.solo;else b.mixer.mute=!b.mixer.mute;if(commit_)commit_(std::move(before),solo?"Toggle bus solo":"Toggle bus mute");syncBusControls();}

    void refreshAssist(){
        suggestions_=analyzeProductionContext(project_);const int previous=suggestionChoice_.getSelectedId();suggestionChoice_.clear(juce::dontSendNotification);int id=1;for(auto const&s:suggestions_)suggestionChoice_.addItem(juce::String(s.title),id++);if(!suggestions_.empty())suggestionChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(suggestions_.size())?previous:1,juce::dontSendNotification);syncSuggestionReport();
    }
    void refreshCommands(){commands_=workflowCommands();commandChoice_.clear(juce::dontSendNotification);int id=1;for(auto const&c:commands_)commandChoice_.addItem(juce::String(c.title),id++);if(!commands_.empty())commandChoice_.setSelectedId(1,juce::dontSendNotification);}
    void syncSuggestionReport(){
        std::ostringstream out;auto issues=validateProject(project_);out<<"Project Health: "<<issues.size()<<" issue(s)\n";for(std::size_t i=0;i<std::min<std::size_t>(issues.size(),5);++i)out<<(issues[i].error?"ERROR: ":"WARN: ")<<issues[i].message<<"\n";
        const int index=suggestionChoice_.getSelectedId()-1;if(index>=0&&index<static_cast<int>(suggestions_.size())){auto const&s=suggestions_[static_cast<std::size_t>(index)];out<<"\nSuggestion: "<<s.title<<"\n"<<s.detail<<"\nAction: "<<s.actionLabel;}
        report_.setText(juce::String(out.str()),false);
    }
    void applySuggestion(){const int index=suggestionChoice_.getSelectedId()-1;if(index<0||index>=static_cast<int>(suggestions_.size()))return;Project before=project_;std::string result;if(!applyAssistantSuggestion(project_,suggestions_[static_cast<std::size_t>(index)],result)){syncSuggestionReport();return;}if(commit_)commit_(std::move(before),"Apply production suggestion");refresh();}
    void runSelectedCommand(){const int index=commandChoice_.getSelectedId()-1;if(index<0||index>=static_cast<int>(commands_.size()))return;runWorkflow(commands_[static_cast<std::size_t>(index)].id);}
    void runWorkflow(const std::string&id){Project before=project_;std::string result;if(!executeWorkflowCommand(project_,id,0,result))return;if(commit_)commit_(std::move(before),"Run workflow command");refresh();}

    Project&project_;PlayheadFn playhead_;CommitFn commit_;bool suppress_=false,busGesture_=false;Project busBefore_;
    std::vector<AssistantSuggestion>suggestions_;std::vector<WorkflowCommand>commands_;
    juce::ComboBox targetChoice_,routeChoice_,busChoice_,suggestionChoice_,commandChoice_;
    juce::Slider value_,busVolume_,busPan_;
    juce::TextButton writePoint_,clearLane_,busMute_,busSolo_,createBus_,refreshAssist_,applySuggestion_,executeCommand_;
    juce::TextEditor report_;
};

} // namespace flowdaw::juceui
