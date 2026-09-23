#include "flowdaw/AppSettings.hpp"
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/ChopEditing.hpp"
#include "flowdaw/Export.hpp"
#include "flowdaw/SampleAnalysis.hpp"
#include "flowdaw/TimeStretch.hpp"
#include "flowdaw/Wav.hpp"
#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/NativeDrums.hpp"
#include "flowdaw/PluginSafety.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/SessionRecovery.hpp"
#include "flowdaw/Undo.hpp"
#include "JuceEditingSurface.hpp"
#include "JucePianoSamplerSurface.hpp"
#include "JuceStepSequencerSurface.hpp"
#include "JuceAutomationAssistSurface.hpp"
#include "JuceSampleBrowser.hpp"
#include "JuceTheme.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace {
using namespace flowdaw;
constexpr int kMaxDeviceBlock=16384;

void addHostFormats(juce::AudioPluginFormatManager&m){
#if JUCE_PLUGINHOST_VST3
    m.addFormat(std::make_unique<juce::VST3PluginFormat>());
#endif
#if JUCE_PLUGINHOST_AU && JUCE_MAC
    m.addFormat(std::make_unique<juce::AudioUnitPluginFormat>());
#endif
}

Project makeStarterProject(){
    Project p;p.name="Untitled Beat";p.transport.bpm=90.0;
    Track audio;audio.name="Audio 1";p.tracks.push_back(std::move(audio));

    Pattern drums;drums.name="Pattern 1";drums.stepCount=16;drums.stepsPerBeat=4;
    const std::array<std::pair<const char*,const char*>,3> kit={{{"Kick","kick"},{"Snare","snare"},{"Hat","hat"}}};
    for(auto const&choice:kit){
        SampleAsset sample;sample.name=std::string("FLOW ")+choice.first;sample.nativeKey=choice.second;sample.audio=std::make_shared<AudioBuffer>(makeNativeDrum(choice.second,p.sampleRate));const Id sid=sample.id;p.samples.push_back(std::move(sample));
        DrumLane lane;lane.name=choice.first;lane.sampleId=sid;lane.steps.resize(16);drums.lanes.push_back(std::move(lane));
    }
    drums.lanes[0].steps[0].active=true;drums.lanes[0].steps[8].active=true;
    drums.lanes[1].steps[4].active=true;drums.lanes[1].steps[12].active=true;
    for(int i=0;i<16;i+=2){auto&step=drums.lanes[2].steps[static_cast<std::size_t>(i)];step.active=true;step.velocity=(i%4==0)?0.72f:0.54f;}
    const Id drumsId=drums.id;p.patterns.push_back(std::move(drums));Track drumTrack;drumTrack.name="Drums";PatternPlacement drumPlacement;drumPlacement.patternId=drumsId;drumPlacement.repeats=8;drumTrack.patternClips.push_back(drumPlacement);p.tracks.push_back(std::move(drumTrack));

    Pattern melody;melody.name="Melody 1";melody.stepCount=32;melody.stepsPerBeat=4;melody.instrument.enabled=true;melody.instrument.type="flow_keys";melody.instrument.gain=0.78f;melody.instrument.attackMs=4.0f;melody.instrument.releaseMs=120.0f;melody.scaleRoot=0;melody.scaleType="minor";melody.midiGridTicks=kPPQ/4;melody.midiDefaultLengthTicks=kPPQ/2;
    const Id melodyId=melody.id;p.patterns.push_back(std::move(melody));Track instrument;instrument.name="INSTRUMENT";PatternPlacement melodyPlacement;melodyPlacement.patternId=melodyId;melodyPlacement.repeats=4;instrument.patternClips.push_back(melodyPlacement);p.tracks.push_back(std::move(instrument));
    return p;
}

Project makeBlankProject(){
    Project p;p.name="Blank Project";p.transport.bpm=120.0;Track audio;audio.name="Audio 1";p.tracks.push_back(std::move(audio));return p;
}
Project makeBeatTemplate(const std::string&name,double bpm,float swing){
    auto p=makeStarterProject();p.name=name;p.transport.bpm=bpm;if(!p.patterns.empty())p.patterns.front().swing=swing;return p;
}

std::string pluginStateHex(juce::AudioProcessor&processor){
    juce::MemoryBlock block;processor.getStateInformation(block);static constexpr char hex[]="0123456789ABCDEF";const auto*bytes=static_cast<const unsigned char*>(block.getData());std::string out(block.getSize()*2,'0');for(std::size_t i=0;i<block.getSize();++i){out[i*2]=hex[bytes[i]>>4];out[i*2+1]=hex[bytes[i]&15];}return out;
}
juce::MemoryBlock decodePluginStateHex(const std::string&state){
    auto nibble=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return-1;};juce::MemoryBlock block;if(state.empty()||state.size()%2)return block;block.setSize(state.size()/2,false);auto*out=static_cast<unsigned char*>(block.getData());for(std::size_t i=0;i<state.size()/2;++i){const int hi=nibble(state[i*2]),lo=nibble(state[i*2+1]);if(hi<0||lo<0){block.reset();return block;}out[i]=static_cast<unsigned char>((hi<<4)|lo);}return block;
}

class PluginEditorWindow final:public juce::DocumentWindow,private juce::Timer{
public:
    using StateFn=std::function<void(const std::string&)>;
    using CloseFn=std::function<void()>;
    explicit PluginEditorWindow(std::unique_ptr<juce::AudioPluginInstance> p,StateFn stateFn={},CloseFn closeFn={})
        :DocumentWindow("FLOWDAW Plugin - "+p->getName(),juce::Colours::darkgrey,DocumentWindow::closeButton),plugin_(std::move(p)),stateFn_(std::move(stateFn)),closeFn_(std::move(closeFn)){
        auto*editor=plugin_->createEditorIfNeeded();if(editor==nullptr)editor=new juce::GenericAudioProcessorEditor(*plugin_);
        setUsingNativeTitleBar(true);setResizable(true,true);setContentOwned(editor,true);centreWithSize(std::max(420,getWidth()),std::max(300,getHeight()));lastState_=pluginStateHex(*plugin_);if(stateFn_)startTimer(150);setVisible(true);
    }
    ~PluginEditorWindow()override{finishSession();}
    void closeButtonPressed()override{finishSession();setVisible(false);}
private:
    void timerCallback()override{if(!plugin_||!stateFn_||closed_)return;auto state=pluginStateHex(*plugin_);if(state!=lastState_){lastState_=state;stateFn_(state);}}
    void finishSession(){if(closed_)return;timerCallback();closed_=true;stopTimer();if(closeFn_)closeFn_();}
    std::unique_ptr<juce::AudioPluginInstance>plugin_;StateFn stateFn_;CloseFn closeFn_;std::string lastState_;bool closed_=false;
};

class MainComponent final:public juce::Component,public juce::DragAndDropContainer,public juce::DragAndDropTarget,private juce::KeyListener,private juce::Timer,private juce::AudioIODeviceCallback{
public:
    MainComponent(){
        pluginHost_=std::make_shared<PluginHost>();pluginHost_->registerBackend(makeJucePluginBackend());engine_.setPluginHost(pluginHost_);
        const auto config=defaultSettingsDirectory();settingsPath_=config/"settings.conf";safetyPath_=config/"plugin-safety.conf";recovery_=std::make_unique<SessionRecovery>(config/"recovery");const bool firstRun=!std::filesystem::exists(settingsPath_);settings_=firstRun?defaultAppSettings():loadAppSettings(settingsPath_);safety_.load(safetyPath_);
        const auto recoveryInfo=recovery_->inspect();bool loadedRecovery=false,loadedLast=false;
        if(settings_.restoreLastSession&&recoveryInfo.available)try{project_=recovery_->loadRecovered(true);projectPath_=recoveryInfo.originalProject;loadedRecovery=true;recoveredAtStartup_=true;preserveRecoveryOnExit_=true;}catch(...){}
        if(!loadedRecovery&&!settings_.lastProjectPath.empty()&&std::filesystem::exists(settings_.lastProjectPath))try{project_=ProjectSerializer::load(settings_.lastProjectPath,true);projectPath_=settings_.lastProjectPath;loadedLast=true;}catch(...){}
        if(!loadedRecovery&&!loadedLast)project_=makeStarterProject();
        try{recovery_->autosave(project_,projectPath_);}catch(...){} 
        juce::String err=deviceManager_.initialise(2,2,nullptr,true);auto setup=deviceManager_.getAudioDeviceSetup();setup.sampleRate=settings_.audio.preferredSampleRate;setup.bufferSize=static_cast<int>(sanitizeBufferSize(settings_.audio.bufferSize));
        if(!settings_.audio.inputDevice.empty())setup.inputDeviceName=settings_.audio.inputDevice;if(!settings_.audio.outputDevice.empty())setup.outputDeviceName=settings_.audio.outputDevice;deviceManager_.setAudioDeviceSetup(setup,true);
        selector_=std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager_,0,2,0,2,true,true,true,false);addAndMakeVisible(*selector_);deviceManager_.addAudioCallback(this);
        addHostFormats(formatManager_);
        title_.setText("FLOWDAW Studio",juce::dontSendNotification);title_.setFont(juce::Font(24.0f,juce::Font::bold));title_.setColour(juce::Label::textColourId,juce::Colour(0xfff4f7fb));addAndMakeVisible(title_);
        status_.setText(recoveredAtStartup_?"Recovered previous autosave; Save, New or Open confirms the session":(err.isEmpty()?"JUCE AudioDeviceManager drives FLOWDAW core":"Audio: "+err),juce::dontSendNotification);addAndMakeVisible(status_);
        projectLabel_.setText(recoveredAtStartup_?(projectPath_.empty()?juce::String("Recovered: Untitled Beat"):juce::String("Recovered: ")+juce::String(projectPath_.filename().string())):(projectPath_.empty()?juce::String("Untitled Beat • starter project"):juce::String("Project: ")+juce::String(projectPath_.filename().string())),juce::dontSendNotification);projectLabel_.setComponentID("project-identity");projectLabel_.setFont(juce::Font(14.0f,juce::Font::bold));addAndMakeVisible(projectLabel_);
        newProject_.setButtonText("New / Template");newProject_.onClick=[this]{showNewProjectMenu();};newProject_.setTooltip("Ctrl/Cmd+N");addAndMakeVisible(newProject_);
        loadProject_.setButtonText("Open .flow");loadProject_.onClick=[this]{chooseProject();};addAndMakeVisible(loadProject_);
        importWav_.setButtonText("Import WAV");importWav_.onClick=[this]{chooseWav();};addAndMakeVisible(importWav_);
        analyzeSample_.setButtonText("Analyze");analyzeSample_.onClick=[this]{analyzeSelectedSample();};addAndMakeVisible(analyzeSample_);
        chop8_.setButtonText("CHOP 8");chop8_.onClick=[this]{equalChopSelected(8);};addAndMakeVisible(chop8_);
        autoChop_.setButtonText("Auto Chop");autoChop_.onClick=[this]{autoChopSelected();};addAndMakeVisible(autoChop_);
        chopBeat_.setButtonText("Beat Chop");chopBeat_.onClick=[this]{beatGridChopSelected(1);};addAndMakeVisible(chopBeat_);
        chopBar_.setButtonText("Bar Chop");chopBar_.onClick=[this]{beatGridChopSelected(4);};addAndMakeVisible(chopBar_);
        matchBpm_.setButtonText("Match BPM");matchBpm_.onClick=[this]{matchSelectedSampleBpm();};addAndMakeVisible(matchBpm_);
        exportMix_.setButtonText("Export Mix");exportMix_.onClick=[this]{chooseMixExport();};addAndMakeVisible(exportMix_);
        exportStems_.setButtonText("Export Stems");exportStems_.onClick=[this]{chooseStemExport();};addAndMakeVisible(exportStems_);
        play_.setButtonText("Play");play_.setComponentID("transport-play");play_.onClick=[this]{togglePlayback();};addAndMakeVisible(play_);
        bpmMinus_.setButtonText("- BPM");bpmMinus_.onClick=[this]{changeBpm(-1.0);};addAndMakeVisible(bpmMinus_);
        bpmPlus_.setButtonText("+ BPM");bpmPlus_.onClick=[this]{changeBpm(1.0);};addAndMakeVisible(bpmPlus_);
        bpmLabel_.setComponentID("transport-bpm");bpmLabel_.setFont(juce::Font(15.0f,juce::Font::bold));bpmLabel_.setJustificationType(juce::Justification::centred);addAndMakeVisible(bpmLabel_);
        stop_.setButtonText("Stop");stop_.setComponentID("transport-stop");stop_.onClick=[this]{engine_.stop();syncTransportVisualState();status_.setText("Stopped",juce::dontSendNotification);};addAndMakeVisible(stop_);
        scan_.setButtonText("Scan VST3/AU");scan_.onClick=[this]{scanPlugins();};addAndMakeVisible(scan_);
        pluginSearch_.setTextToShowWhenEmpty("Search plugins…",juce::Colour(0xff7f8796));pluginSearch_.onTextChange=[this]{refreshPluginChoice();};addAndMakeVisible(pluginSearch_);
        pluginKindChoice_.addItem("All plugins",1);pluginKindChoice_.addItem("Instruments",2);pluginKindChoice_.addItem("Effects",3);pluginKindChoice_.setSelectedId(1,juce::dontSendNotification);pluginKindChoice_.onChange=[this]{refreshPluginChoice();};addAndMakeVisible(pluginKindChoice_);
        openEditor_.setButtonText("Preview Scanned Plugin");openEditor_.onClick=[this]{openSelectedEditor();};addAndMakeVisible(openEditor_);
        addAndMakeVisible(pluginChoice_);pluginChoice_.setTextWhenNothingSelected("No plugin matches");
        refreshRackTargets();rackTargetChoice_.setSelectedId(1,juce::dontSendNotification);rackTargetChoice_.onChange=[this]{if(!suppressRackCallbacks_){syncMixerTargetFromRack();refreshRackControls();}};addAndMakeVisible(rackTargetChoice_);
        rackPluginChoice_.setTextWhenNothingSelected("Rack empty");rackPluginChoice_.onChange=[this]{syncRackControls();};addAndMakeVisible(rackPluginChoice_);
        addRackGain_.setButtonText("+ FLOW Gain");addRackGain_.onClick=[this]{addBuiltinToRack("flow.gain");};addAndMakeVisible(addRackGain_);
        addRackClip_.setButtonText("+ Soft Clip");addRackClip_.onClick=[this]{addBuiltinToRack("flow.softclip");};addAndMakeVisible(addRackClip_);
        addRackWidth_.setButtonText("+ Width");addRackWidth_.onClick=[this]{addBuiltinToRack("flow.width");};addAndMakeVisible(addRackWidth_);
        addRackExternal_.setButtonText("+ Plugin");addRackExternal_.onClick=[this]{addSelectedEffectToRack();};addAndMakeVisible(addRackExternal_);
        rackMoveUp_.setButtonText("Up");rackMoveUp_.onClick=[this]{moveRackPlugin(-1);};addAndMakeVisible(rackMoveUp_);
        rackMoveDown_.setButtonText("Down");rackMoveDown_.onClick=[this]{moveRackPlugin(1);};addAndMakeVisible(rackMoveDown_);
        rackEnabled_.setButtonText("Disable");rackEnabled_.onClick=[this]{toggleRackEnabled();};addAndMakeVisible(rackEnabled_);
        rackBypass_.setButtonText("Bypass");rackBypass_.onClick=[this]{toggleRackBypass();};addAndMakeVisible(rackBypass_);
        rackRemove_.setButtonText("Remove");rackRemove_.onClick=[this]{removeRackPlugin();};addAndMakeVisible(rackRemove_);
        openRackEditor_.setButtonText("Open Insert");openRackEditor_.onClick=[this]{openRackEditor();};addAndMakeVisible(openRackEditor_);
        rackWet_.setRange(0.0,1.0,0.01);rackWet_.setSliderStyle(juce::Slider::LinearHorizontal);rackWet_.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,22);rackWet_.setTextValueSuffix(" wet");rackWet_.onDragStart=[this]{beginRackGesture();};rackWet_.onValueChange=[this]{applyRackSliders();};rackWet_.onDragEnd=[this]{endRackGesture();};addAndMakeVisible(rackWet_);
        rackParam_.setSliderStyle(juce::Slider::LinearHorizontal);rackParam_.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,22);rackParam_.onDragStart=[this]{beginRackGesture();};rackParam_.onValueChange=[this]{applyRackSliders();};rackParam_.onDragEnd=[this]{endRackGesture();};addAndMakeVisible(rackParam_);
        rackParamLabel_.setText("Parameter",juce::dontSendNotification);addAndMakeVisible(rackParamLabel_);
        addAndMakeVisible(trackChoice_);trackChoice_.setTextWhenNothingSelected("No track selected");
        setInstrument_.setButtonText("Set Instrument");setInstrument_.onClick=[this]{assignSelectedInstrument();};addAndMakeVisible(setInstrument_);
        clearInstrument_.setButtonText("Clear Instrument");clearInstrument_.onClick=[this]{clearSelectedInstrument();};addAndMakeVisible(clearInstrument_);
        addTrackFx_.setButtonText("Add Track FX");addTrackFx_.onClick=[this]{addSelectedEffect(false);};addAndMakeVisible(addTrackFx_);
        addMasterFx_.setButtonText("Add Master FX");addMasterFx_.onClick=[this]{addSelectedEffect(true);};addAndMakeVisible(addMasterFx_);
        saveProject_.setButtonText("Save Project");saveProject_.onClick=[this]{saveProject();};addAndMakeVisible(saveProject_);
        undoButton_.setButtonText("Undo");undoButton_.onClick=[this]{undoEdit();};addAndMakeVisible(undoButton_);
        redoButton_.setButtonText("Redo");redoButton_.onClick=[this]{redoEdit();};addAndMakeVisible(redoButton_);
        commandPalette_.setButtonText("Commands");commandPalette_.setTooltip("Ctrl/Cmd+K");commandPalette_.onClick=[this]{showCommandPalette();};addAndMakeVisible(commandPalette_);
        trackChoice_.onChange=[this]{refreshMixerTargets();syncMixerControls();refreshMixerRoutingControls();syncRackTargetToMixer();refreshRackControls();};
        mixerSectionLabel_.setText("MIXER • ACTIVE CHANNEL",juce::dontSendNotification);mixerSectionLabel_.setFont(juce::Font(14.0f,juce::Font::bold));mixerSectionLabel_.setColour(juce::Label::textColourId,juce::Colour(0xffdfe7f3));addAndMakeVisible(mixerSectionLabel_);
        mixerTargetChoice_.setTextWhenNothingSelected("No mixer target");mixerTargetChoice_.onChange=[this]{if(!suppressMixerCallbacks_){syncMixerControls();refreshMixerRoutingControls();syncRackTargetToMixer();refreshRackControls();}};addAndMakeVisible(mixerTargetChoice_);
        mixerVolume_.setRange(0.0,2.0,0.01);mixerVolume_.setSliderStyle(juce::Slider::LinearHorizontal);mixerVolume_.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);mixerVolume_.onDragStart=[this]{beginMixerGesture();};mixerVolume_.onValueChange=[this]{applyMixerSliders();};mixerVolume_.onDragEnd=[this]{endMixerGesture();};addAndMakeVisible(mixerVolume_);
        mixerPan_.setRange(-1.0,1.0,0.01);mixerPan_.setSliderStyle(juce::Slider::LinearHorizontal);mixerPan_.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);mixerPan_.onDragStart=[this]{beginMixerGesture();};mixerPan_.onValueChange=[this]{applyMixerSliders();};mixerPan_.onDragEnd=[this]{endMixerGesture();};addAndMakeVisible(mixerPan_);
        mixerVolumeLabel_.setText("VOLUME",juce::dontSendNotification);mixerPanLabel_.setText("PAN",juce::dontSendNotification);addAndMakeVisible(mixerVolumeLabel_);addAndMakeVisible(mixerPanLabel_);
        muteTrack_.setButtonText("Mute");muteTrack_.onClick=[this]{toggleMixerFlag(false);};addAndMakeVisible(muteTrack_);
        soloTrack_.setButtonText("Solo");soloTrack_.onClick=[this]{toggleMixerFlag(true);};addAndMakeVisible(soloTrack_);
        trackMeterLabel_.setText("CHANNEL METER",juce::dontSendNotification);addAndMakeVisible(trackMeterLabel_);addAndMakeVisible(trackMeter_);
        mixerRoutingLabel_.setText("ROUTING / SENDS",juce::dontSendNotification);mixerRoutingLabel_.setColour(juce::Label::textColourId,juce::Colour(0xffaeb8c8));addAndMakeVisible(mixerRoutingLabel_);
        mixerOutputChoice_.setTextWhenNothingSelected("Output route");mixerOutputChoice_.onChange=[this]{if(!suppressMixerCallbacks_)setMixerOutputRoute();};addAndMakeVisible(mixerOutputChoice_);
        mixerSendBusChoice_.setTextWhenNothingSelected("Send bus");mixerSendBusChoice_.onChange=[this]{if(!suppressMixerCallbacks_)syncMixerSendControls();};addAndMakeVisible(mixerSendBusChoice_);
        mixerSendGain_.setRange(0.0,2.0,0.01);mixerSendGain_.setSliderStyle(juce::Slider::LinearHorizontal);mixerSendGain_.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,22);mixerSendGain_.setTextValueSuffix(" send");addAndMakeVisible(mixerSendGain_);
        mixerSendPre_.setButtonText("Pre");addAndMakeVisible(mixerSendPre_);
        mixerSetSend_.setButtonText("Set Send");mixerSetSend_.onClick=[this]{setMixerSend();};addAndMakeVisible(mixerSetSend_);
        mixerRemoveSend_.setButtonText("Remove");mixerRemoveSend_.onClick=[this]{removeMixerSend();};addAndMakeVisible(mixerRemoveSend_);
        auto commitEdit=[this](Project before,std::string name){undo_.commit(std::move(before),project_,name);publishEdit(juce::String(name));};
        arrangement_=std::make_unique<juceui::ArrangementComponent>(project_,commitEdit);addAndMakeVisible(*arrangement_);
        piano_=std::make_unique<juceui::PianoRollComponent>(project_,engine_,commitEdit);addChildComponent(*piano_);
        sampler_=std::make_unique<juceui::SamplerComponent>(project_,engine_,commitEdit,[this](Id sampleId,Id sliceId){recordChopTrigger(sampleId,sliceId);});addChildComponent(*sampler_);
        sequencer_=std::make_unique<juceui::StepSequencerComponent>(project_,commitEdit);addChildComponent(*sequencer_);
        automationAssist_=std::make_unique<juceui::AutomationAssistComponent>(project_,[this]{return MusicalTime::samplesToTicks(engine_.playheadSamples(),project_.transport.bpm,engine_.sampleRate());},commitEdit);addChildComponent(*automationAssist_);
        sampleBrowser_=std::make_unique<juceui::SampleBrowserComponent>(
            settings_,
            [this](const std::filesystem::path& path){importWavFile(path);},
            [this](std::shared_ptr<AudioBuffer> audio){if(!audio||audio->frames()<=0)return;engine_.stopPreviews();engine_.triggerPreview(audio,0,audio->frames(),0.85f,0.0f,0);},
            [this]{engine_.stopPreviews();},
            [this]{try{saveAppSettings(settings_,settingsPath_);}catch(const std::exception&e){status_.setText("Sample library settings failed: "+juce::String(e.what()),juce::dontSendNotification);}}
        );addAndMakeVisible(*sampleBrowser_);
        arrangementTab_.setButtonText("Arrangement");arrangementTab_.onClick=[this]{setEditorMode(EditorMode::Arrangement);};addAndMakeVisible(arrangementTab_);
        pianoTab_.setButtonText("Piano Roll");pianoTab_.onClick=[this]{setEditorMode(EditorMode::Piano);};addAndMakeVisible(pianoTab_);
        sequencerTab_.setButtonText("Sequencer");sequencerTab_.onClick=[this]{setEditorMode(EditorMode::Step);};addAndMakeVisible(sequencerTab_);
        automationTab_.setButtonText("Automation");automationTab_.onClick=[this]{setEditorMode(EditorMode::Automation);};addAndMakeVisible(automationTab_);
        samplerTab_.setButtonText("Sampler");samplerTab_.onClick=[this]{setEditorMode(EditorMode::Sampler);};addAndMakeVisible(samplerTab_);
        for(auto*tab:{&arrangementTab_,&pianoTab_,&sequencerTab_,&automationTab_,&samplerTab_})tab->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff315d9b));
        addAndMakeVisible(patternChoice_);patternChoice_.setTextWhenNothingSelected("No MIDI pattern");patternChoice_.onChange=[this]{syncPatternEditors();};
        addAndMakeVisible(sampleChoice_);sampleChoice_.setTextWhenNothingSelected("No audio sample");sampleChoice_.onChange=[this]{syncSamplerSample();};
        bankPrev_.setButtonText("Bank -");bankPrev_.onClick=[this]{if(sampler_)sampler_->previousBank();};addAndMakeVisible(bankPrev_);
        bankNext_.setButtonText("Bank +");bankNext_.onClick=[this]{if(sampler_)sampler_->nextBank();};addAndMakeVisible(bankNext_);
        padName_.setTextToShowWhenEmpty("Pad name",juce::Colour(0xff8f96a3));addAndMakeVisible(padName_);
        renamePad_.setButtonText("Rename");renamePad_.onClick=[this]{if(!sampler_||!sampler_->renameSelectedPad(padName_.getText().toStdString()))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(renamePad_);
        padGainMinus_.setButtonText("Gain -");padGainMinus_.onClick=[this]{if(!sampler_||!sampler_->adjustSelectedPad(-0.05f,0.0f,0))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(padGainMinus_);
        padGainPlus_.setButtonText("Gain +");padGainPlus_.onClick=[this]{if(!sampler_||!sampler_->adjustSelectedPad(0.05f,0.0f,0))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(padGainPlus_);
        padPanMinus_.setButtonText("Pan -");padPanMinus_.onClick=[this]{if(!sampler_||!sampler_->adjustSelectedPad(0.0f,-0.10f,0))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(padPanMinus_);
        padPanPlus_.setButtonText("Pan +");padPanPlus_.onClick=[this]{if(!sampler_||!sampler_->adjustSelectedPad(0.0f,0.10f,0))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(padPanPlus_);
        padChokeMinus_.setButtonText("Choke -");padChokeMinus_.onClick=[this]{if(!sampler_||!sampler_->adjustSelectedPad(0.0f,0.0f,-1))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(padChokeMinus_);
        padChokePlus_.setButtonText("Choke +");padChokePlus_.onClick=[this]{if(!sampler_||!sampler_->adjustSelectedPad(0.0f,0.0f,1))status_.setText("Select a pad first",juce::dontSendNotification);};addAndMakeVisible(padChokePlus_);
        stopPreview_.setButtonText("Stop Preview");stopPreview_.onClick=[this]{engine_.stopPreviews();status_.setText("Preview stopped",juce::dontSendNotification);};addAndMakeVisible(stopPreview_);
        recChops_.setButtonText("REC CHOPS");recChops_.onClick=[this]{toggleChopRecording();};addAndMakeVisible(recChops_);
        chopGridChoice_.addItem("Grid 1/8",1);chopGridChoice_.addItem("Grid 1/16",2);chopGridChoice_.addItem("Grid 1/32",3);chopGridChoice_.setSelectedId(2,juce::dontSendNotification);chopGridChoice_.onChange=[this]{changeChopGrid();};addAndMakeVisible(chopGridChoice_);
        chopQuantizeStrength_.setRange(0.0,100.0,1.0);chopQuantizeStrength_.setSliderStyle(juce::Slider::LinearHorizontal);chopQuantizeStrength_.setTextBoxStyle(juce::Slider::TextBoxRight,false,54,22);chopQuantizeStrength_.setTextValueSuffix("% Q");chopQuantizeStrength_.onDragStart=[this]{beginChopGesture();};chopQuantizeStrength_.onValueChange=[this]{applyChopControls();};chopQuantizeStrength_.onDragEnd=[this]{endChopGesture();};addAndMakeVisible(chopQuantizeStrength_);
        chopHumanizeStrength_.setRange(0.0,100.0,1.0);chopHumanizeStrength_.setSliderStyle(juce::Slider::LinearHorizontal);chopHumanizeStrength_.setTextBoxStyle(juce::Slider::TextBoxRight,false,54,22);chopHumanizeStrength_.setTextValueSuffix("% H");chopHumanizeStrength_.onDragStart=[this]{beginChopGesture();};chopHumanizeStrength_.onValueChange=[this]{applyChopControls();};chopHumanizeStrength_.onDragEnd=[this]{endChopGesture();};addAndMakeVisible(chopHumanizeStrength_);
        chopReset_.setButtonText("Reset Feel");chopReset_.onClick=[this]{resetLatestChops();};addAndMakeVisible(chopReset_);
        recAudio_.setButtonText("REC Audio");recAudio_.onClick=[this]{toggleAudioRecording();};addAndMakeVisible(recAudio_);
        monitorInput_.setButtonText("Monitor");monitorInput_.onClick=[this]{toggleInputMonitor();};addAndMakeVisible(monitorInput_);
        prevTake_.setButtonText("Take -");prevTake_.onClick=[this]{cycleTake(-1);};addAndMakeVisible(prevTake_);
        nextTake_.setButtonText("Take +");nextTake_.onClick=[this]{cycleTake(1);};addAndMakeVisible(nextTake_);
        refreshTrackChoice();refreshMixerTargets();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshMixerRoutingControls();syncRackTargetToMixer();refreshRackControls();syncChopControls();updateBpmLabel();setEditorMode(EditorMode::Arrangement);
        note_.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(note_);
        meterLabel_.setText("MASTER • waiting for audio",juce::dontSendNotification);meterLabel_.setJustificationType(juce::Justification::centredRight);addAndMakeVisible(meterLabel_);
        applyShellTheme();
        setWantsKeyboardFocus(true);installShortcutListeners(*this);setSize(1440,1040);startTimer(100);if(firstRun){saveDeviceSettings();auto safe=juce::Component::SafePointer<MainComponent>(this);juce::MessageManager::callAsync([safe]()mutable{if(auto*self=safe.getComponent())self->showFirstRunOnboarding();});}
    }
    ~MainComponent()override{clearShellTheme();if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();pluginWindow_.reset();deviceManager_.removeAudioCallback(this);engine_.stop();engine_.collectRetiredGraphs();saveDeviceSettings();saveSafety();if(recovery_&&!preserveRecoveryOnExit_)try{recovery_->markCleanExit();}catch(...){}}
    void paint(juce::Graphics&g)override{
        auto bounds=getLocalBounds().toFloat();
        juce::ColourGradient backdrop(juceui::FlowTheme::canvasTop(),bounds.getX(),bounds.getY(),juceui::FlowTheme::canvasBottom(),bounds.getRight(),bounds.getBottom(),false);
        backdrop.addColour(0.34,juce::Colour(0xff151027));
        backdrop.addColour(0.72,juce::Colour(0xff071319));
        g.setGradientFill(backdrop);g.fillRect(bounds);

        juce::ColourGradient roseGlow(juceui::FlowTheme::accent().withAlpha(0.18f),72.0f,20.0f,juce::Colours::transparentBlack,430.0f,300.0f,true);
        g.setGradientFill(roseGlow);g.fillEllipse(-190.0f,-180.0f,720.0f,560.0f);
        juce::ColourGradient aquaGlow(juceui::FlowTheme::aqua().withAlpha(0.10f),bounds.getRight()-90.0f,90.0f,juce::Colours::transparentBlack,bounds.getRight()-520.0f,420.0f,true);
        g.setGradientFill(aquaGlow);g.fillEllipse(bounds.getRight()-650.0f,-150.0f,760.0f,620.0f);

        auto shell=bounds.reduced(12.0f);shell.setHeight(std::min(170.0f,shell.getHeight()));
        juce::ColourGradient deck(juce::Colour(0xff171425).withAlpha(0.94f),shell.getX(),shell.getY(),juce::Colour(0xff0d151c).withAlpha(0.91f),shell.getRight(),shell.getBottom(),false);
        deck.addColour(0.52,juce::Colour(0xff1b1426).withAlpha(0.86f));
        g.setGradientFill(deck);g.fillRoundedRectangle(shell,juceui::FlowTheme::radiusPanel);
        g.setColour(juceui::FlowTheme::borderSubtle().withAlpha(0.74f));g.drawRoundedRectangle(shell,juceui::FlowTheme::radiusPanel,1.0f);
        g.setColour(juceui::FlowTheme::borderSubtle().withAlpha(0.52f));
        g.drawLine(shell.getX()+18.0f,shell.getY()+48.0f,shell.getRight()-18.0f,shell.getY()+48.0f,1.0f);
        g.drawLine(shell.getX()+18.0f,shell.getY()+96.0f,shell.getRight()-18.0f,shell.getY()+96.0f,1.0f);

        auto signature=juce::Rectangle<float>(shell.getX()+10.0f,shell.getY()+12.0f,5.0f,76.0f);
        juce::ColourGradient signatureFill(juceui::FlowTheme::accentHot(),signature.getX(),signature.getY(),juceui::FlowTheme::aqua(),signature.getX(),signature.getBottom(),false);
        g.setGradientFill(signatureFill);g.fillRoundedRectangle(signature,2.5f);
    }
    void resized()override{
        auto r=getLocalBounds().reduced(16);

        auto header=r.removeFromTop(46);
        title_.setBounds(header.removeFromLeft(215).reduced(4,2));
        projectLabel_.setBounds(header.removeFromLeft(330).reduced(6,3));
        header.removeFromLeft(12);
        commandPalette_.setBounds(header.removeFromRight(104).reduced(3,5));
        saveProject_.setBounds(header.removeFromRight(106).reduced(3,5));
        loadProject_.setBounds(header.removeFromRight(96).reduced(3,5));
        newProject_.setBounds(header.removeFromRight(112).reduced(3,5));
        importWav_.setBounds(header.removeFromRight(102).reduced(3,5));

        auto transport=r.removeFromTop(48);
        transport.removeFromLeft(8);
        play_.setBounds(transport.removeFromLeft(78).reduced(3,5));
        stop_.setBounds(transport.removeFromLeft(68).reduced(3,5));
        transport.removeFromLeft(12);
        bpmMinus_.setBounds(transport.removeFromLeft(58).reduced(3,5));
        bpmLabel_.setBounds(transport.removeFromLeft(92).reduced(3,5));
        bpmPlus_.setBounds(transport.removeFromLeft(58).reduced(3,5));
        transport.removeFromLeft(14);
        undoButton_.setBounds(transport.removeFromLeft(70).reduced(3,5));
        redoButton_.setBounds(transport.removeFromLeft(70).reduced(3,5));
        transport.removeFromLeft(16);
        status_.setBounds(transport.reduced(8,4));

        auto workspace=r.removeFromTop(44);
        workspace.removeFromLeft(8);
        arrangementTab_.setBounds(workspace.removeFromLeft(112).reduced(3,4));
        pianoTab_.setBounds(workspace.removeFromLeft(98).reduced(3,4));
        sequencerTab_.setBounds(workspace.removeFromLeft(100).reduced(3,4));
        automationTab_.setBounds(workspace.removeFromLeft(104).reduced(3,4));
        samplerTab_.setBounds(workspace.removeFromLeft(90).reduced(3,4));
        workspace.removeFromLeft(16);
        patternChoice_.setBounds(workspace.removeFromLeft(230).reduced(3,4));
        sampleChoice_.setBounds(workspace.removeFromLeft(238).reduced(3,4));
        bankPrev_.setBounds(workspace.removeFromLeft(72).reduced(3,4));
        bankNext_.setBounds(workspace.removeFromLeft(72).reduced(3,4));

        auto context=r.removeFromTop(28);
        note_.setBounds(context.removeFromLeft(std::max(420,context.getWidth()*2/3)).reduced(4,2));
        meterLabel_.setBounds(context.reduced(4,2));

        r.removeFromTop(8);

        auto controls=r.removeFromTop(38);scan_.setBounds(controls.removeFromLeft(135).reduced(3));pluginSearch_.setBounds(controls.removeFromLeft(210).reduced(3));pluginKindChoice_.setBounds(controls.removeFromLeft(125).reduced(3));pluginChoice_.setBounds(controls.removeFromLeft(380).reduced(3));openEditor_.setBounds(controls.removeFromLeft(170).reduced(3));
        auto instrument=r.removeFromTop(38);trackChoice_.setBounds(instrument.removeFromLeft(300).reduced(3));setInstrument_.setBounds(instrument.removeFromLeft(130).reduced(3));clearInstrument_.setBounds(instrument.removeFromLeft(135).reduced(3));addTrackFx_.setBounds(instrument.removeFromLeft(120).reduced(3));addMasterFx_.setBounds(instrument.removeFromLeft(125).reduced(3));
        auto rack=r.removeFromTop(38);rackTargetChoice_.setBounds(rack.removeFromLeft(150).reduced(3));rackPluginChoice_.setBounds(rack.removeFromLeft(205).reduced(3));addRackGain_.setBounds(rack.removeFromLeft(92).reduced(3));addRackClip_.setBounds(rack.removeFromLeft(82).reduced(3));addRackWidth_.setBounds(rack.removeFromLeft(70).reduced(3));addRackExternal_.setBounds(rack.removeFromLeft(78).reduced(3));rackMoveUp_.setBounds(rack.removeFromLeft(38).reduced(3));rackMoveDown_.setBounds(rack.removeFromLeft(45).reduced(3));rackEnabled_.setBounds(rack.removeFromLeft(68).reduced(3));rackBypass_.setBounds(rack.removeFromLeft(62).reduced(3));rackRemove_.setBounds(rack.removeFromLeft(62).reduced(3));openRackEditor_.setBounds(rack.removeFromLeft(90).reduced(3));rackWet_.setBounds(rack.removeFromLeft(120).reduced(3));rackParamLabel_.setBounds(rack.removeFromLeft(60).reduced(3));rackParam_.setBounds(rack.reduced(3));
        auto sampleTools=r.removeFromTop(38);analyzeSample_.setBounds(sampleTools.removeFromLeft(82).reduced(3));chop8_.setBounds(sampleTools.removeFromLeft(72).reduced(3));autoChop_.setBounds(sampleTools.removeFromLeft(88).reduced(3));chopBeat_.setBounds(sampleTools.removeFromLeft(88).reduced(3));chopBar_.setBounds(sampleTools.removeFromLeft(84).reduced(3));matchBpm_.setBounds(sampleTools.removeFromLeft(92).reduced(3));exportMix_.setBounds(sampleTools.removeFromLeft(90).reduced(3));exportStems_.setBounds(sampleTools.removeFromLeft(100).reduced(3));padName_.setBounds(sampleTools.removeFromLeft(118).reduced(3));renamePad_.setBounds(sampleTools.removeFromLeft(70).reduced(3));padGainMinus_.setBounds(sampleTools.removeFromLeft(64).reduced(3));padGainPlus_.setBounds(sampleTools.removeFromLeft(64).reduced(3));padPanMinus_.setBounds(sampleTools.removeFromLeft(60).reduced(3));padPanPlus_.setBounds(sampleTools.removeFromLeft(60).reduced(3));
        auto recordTools=r.removeFromTop(38);stopPreview_.setBounds(recordTools.removeFromLeft(88).reduced(3));recChops_.setBounds(recordTools.removeFromLeft(88).reduced(3));chopGridChoice_.setBounds(recordTools.removeFromLeft(92).reduced(3));chopQuantizeStrength_.setBounds(recordTools.removeFromLeft(160).reduced(3));chopHumanizeStrength_.setBounds(recordTools.removeFromLeft(160).reduced(3));chopReset_.setBounds(recordTools.removeFromLeft(82).reduced(3));recAudio_.setBounds(recordTools.removeFromLeft(88).reduced(3));monitorInput_.setBounds(recordTools.removeFromLeft(80).reduced(3));prevTake_.setBounds(recordTools.removeFromLeft(70).reduced(3));nextTake_.setBounds(recordTools.removeFromLeft(70).reduced(3));padChokeMinus_.setBounds(recordTools.removeFromLeft(74).reduced(3));padChokePlus_.setBounds(recordTools.removeFromLeft(74).reduced(3));

        r.removeFromTop(4);auto editorArea=r.removeFromTop(320);auto browserArea=editorArea.removeFromLeft(std::min(280,editorArea.getWidth()/3));if(sampleBrowser_)sampleBrowser_->setBounds(browserArea.reduced(0,2));editorArea.removeFromLeft(6);if(arrangement_)arrangement_->setBounds(editorArea);if(piano_)piano_->setBounds(editorArea);if(sampler_)sampler_->setBounds(editorArea);if(sequencer_)sequencer_->setBounds(editorArea);if(automationAssist_)automationAssist_->setBounds(editorArea);
        auto mixer=r.removeFromTop(162);auto mixerHeader=mixer.removeFromTop(34);mixerSectionLabel_.setBounds(mixerHeader.removeFromLeft(220).reduced(3));mixerTargetChoice_.setBounds(mixerHeader.removeFromLeft(320).reduced(3));auto mixerControls=mixer.removeFromTop(38);mixerVolumeLabel_.setBounds(mixerControls.removeFromLeft(72).reduced(3));mixerVolume_.setBounds(mixerControls.removeFromLeft(260).reduced(3));mixerPanLabel_.setBounds(mixerControls.removeFromLeft(45).reduced(3));mixerPan_.setBounds(mixerControls.removeFromLeft(220).reduced(3));muteTrack_.setBounds(mixerControls.removeFromLeft(72).reduced(3));soloTrack_.setBounds(mixerControls.removeFromLeft(72).reduced(3));auto mixerRouting=mixer.removeFromTop(42);mixerRoutingLabel_.setBounds(mixerRouting.removeFromLeft(120).reduced(3));mixerOutputChoice_.setBounds(mixerRouting.removeFromLeft(220).reduced(3));mixerSendBusChoice_.setBounds(mixerRouting.removeFromLeft(200).reduced(3));mixerSendGain_.setBounds(mixerRouting.removeFromLeft(180).reduced(3));mixerSendPre_.setBounds(mixerRouting.removeFromLeft(58).reduced(3));mixerSetSend_.setBounds(mixerRouting.removeFromLeft(92).reduced(3));mixerRemoveSend_.setBounds(mixerRouting.removeFromLeft(82).reduced(3));auto mixerMeter=mixer;trackMeterLabel_.setBounds(mixerMeter.removeFromLeft(120).reduced(3));trackMeter_.setBounds(mixerMeter.reduced(3));
        r.removeFromTop(6);selector_->setBounds(r);
    }
private:
    void applyShellTheme(){
        title_.setColour(juce::Label::textColourId,juceui::FlowTheme::textPrimary());
        status_.setColour(juce::Label::textColourId,juceui::FlowTheme::aqua().withAlpha(0.88f));status_.setFont(juce::Font(13.0f,juce::Font::bold));
        projectLabel_.setColour(juce::Label::textColourId,juceui::FlowTheme::textSecondary());
        note_.setColour(juce::Label::textColourId,juceui::FlowTheme::textSecondary());note_.setFont(juce::Font(11.5f,juce::Font::bold));
        meterLabel_.setColour(juce::Label::textColourId,juceui::FlowTheme::textMuted());meterLabel_.setFont(juce::Font(11.0f));
        bpmLabel_.setColour(juce::Label::textColourId,juceui::FlowTheme::textPrimary());
        for(auto*button:{&newProject_,&loadProject_,&importWav_,&saveProject_,&play_,&stop_,&bpmMinus_,&bpmPlus_,&undoButton_,&redoButton_,&commandPalette_,&arrangementTab_,&pianoTab_,&sequencerTab_,&automationTab_,&samplerTab_})button->setLookAndFeel(&shellLookAndFeel_);
        for(auto*choice:{&patternChoice_,&sampleChoice_})choice->setLookAndFeel(&shellLookAndFeel_);
        bpmLabel_.setLookAndFeel(&shellLookAndFeel_);projectLabel_.setLookAndFeel(&shellLookAndFeel_);
    }
    void clearShellTheme(){
        for(auto*button:{&newProject_,&loadProject_,&importWav_,&saveProject_,&play_,&stop_,&bpmMinus_,&bpmPlus_,&undoButton_,&redoButton_,&commandPalette_,&arrangementTab_,&pianoTab_,&sequencerTab_,&automationTab_,&samplerTab_})button->setLookAndFeel(nullptr);
        for(auto*choice:{&patternChoice_,&sampleChoice_})choice->setLookAndFeel(nullptr);
        bpmLabel_.setLookAndFeel(nullptr);projectLabel_.setLookAndFeel(nullptr);
    }
    void audioDeviceAboutToStart(juce::AudioIODevice*device)override{if(!device)return;engine_.configureExternalDevice(static_cast<int>(device->getCurrentSampleRate()),static_cast<unsigned long>(device->getCurrentBufferSizeSamples()),device->getActiveInputChannels().countNumberOfSetBits()>0);engine_.publish(project_);}
    void audioDeviceStopped()override{engine_.configureExternalDevice(engine_.sampleRate(),256,false);}
    void audioDeviceIOCallbackWithContext(const float*const*inputs,int numInputs,float*const*outputs,int numOutputs,int numSamples,const juce::AudioIODeviceCallbackContext&)override{
        if(numSamples<=0)return;if(numSamples>kMaxDeviceBlock){for(int c=0;c<numOutputs;++c)if(outputs[c])juce::FloatVectorOperations::clear(outputs[c],numSamples);return;}
        const bool hasInput=numInputs>0&&inputs&&inputs[0];if(hasInput){for(int i=0;i<numSamples;++i){float v=inputs[0][i];if(numInputs>1&&inputs[1])v=(v+inputs[1][i])*0.5f;monoInput_[static_cast<std::size_t>(i)]=v;}}
        std::fill_n(stereoOutput_.data(),static_cast<std::size_t>(numSamples)*2,0.0f);engine_.processExternalDeviceBlock(hasInput?monoInput_.data():nullptr,stereoOutput_.data(),static_cast<unsigned long>(numSamples));
        for(int c=0;c<numOutputs;++c){if(!outputs[c])continue;if(c<2)for(int i=0;i<numSamples;++i)outputs[c][i]=stereoOutput_[static_cast<std::size_t>(i)*2+static_cast<std::size_t>(c)];else juce::FloatVectorOperations::clear(outputs[c],numSamples);}
    }
    int selectedSampleIndex()const{const int index=sampleChoice_.getSelectedId()-1;return index>=0&&index<static_cast<int>(project_.samples.size())?index:-1;}
    SampleAsset*selectedSample(){const int index=selectedSampleIndex();return index>=0?&project_.samples[static_cast<std::size_t>(index)]:nullptr;}
    bool sampleReferencedByChops(Id sampleId)const{for(auto const&p:project_.patterns)for(auto const&ev:p.chopEvents)if(ev.sampleId==sampleId)return true;return false;}
    Clip*firstClipForSample(Id sampleId){for(auto&t:project_.tracks)for(auto&c:t.clips)if(c.sampleId==sampleId)return &c;return nullptr;}
    void selectSampleId(Id id){for(std::size_t i=0;i<project_.samples.size();++i)if(project_.samples[i].id==id){sampleChoice_.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);syncSamplerSample();return;}}
    Tick playheadTick()const{return MusicalTime::samplesToTicks(engine_.playheadSamples(),project_.transport.bpm,engine_.sampleRate());}
    Track*chopTrack(){for(auto&t:project_.tracks)if(t.name=="CHOPS")return&t;Track t;t.name="CHOPS";project_.tracks.push_back(std::move(t));return&project_.tracks.back();}
    Pattern*latestChopPattern(){for(auto it=project_.patterns.rbegin();it!=project_.patterns.rend();++it)if(!it->chopEvents.empty())return&*it;return nullptr;}
    void startChopRecording(){
        auto*smp=selectedSample();if(!smp||!smp->audio||smp->slices.empty()){status_.setText("Create/select chops before REC CHOPS",juce::dontSendNotification);return;}
        chopRecordBefore_=project_;Pattern take;take.name="Chop Take";take.stepCount=64;take.stepsPerBeat=4;chopRecordPatternId_=take.id;project_.patterns.push_back(std::move(take));auto*t=chopTrack();chopRecordStartTick_=playheadTick();PatternPlacement pp;pp.patternId=chopRecordPatternId_;pp.startTick=chopRecordStartTick_;pp.repeats=1;t->patternClips.push_back(pp);recordingChops_=true;if(!engine_.isPlaying())engine_.play();engine_.publish(project_);if(arrangement_)arrangement_->repaint();status_.setText("REC CHOPS - play 1-4/QWER/ASDF/ZXCV",juce::dontSendNotification);
    }
    void finishChopRecording(){
        if(!recordingChops_)return;recordingChops_=false;auto*p=project_.findPattern(chopRecordPatternId_);if(!p||p->chopEvents.empty()){project_=std::move(chopRecordBefore_);chopRecordPatternId_=0;publishEdit("Empty chop take discarded");return;}const auto hits=p->chopEvents.size();undo_.commit(std::move(chopRecordBefore_),project_,"Record chops");chopRecordPatternId_=0;publishEdit("Recorded "+juce::String(static_cast<int>(hits))+" chop hits");
    }
    void toggleChopRecording(){recordingChops_?finishChopRecording():startChopRecording();}
    void recordChopTrigger(Id sampleId,Id sliceId){
        if(!recordingChops_)return;auto*p=project_.findPattern(chopRecordPatternId_);if(!p)return;const Tick captured=std::max<Tick>(0,playheadTick()-chopRecordStartTick_);ChopEvent ev;ev.recordedTick=captured;ev.tick=captured;ev.sampleId=sampleId;ev.sliceId=sliceId;ev.recordedVelocity=1.0f;ev.velocity=1.0f;p->chopEvents.push_back(ev);const int need=static_cast<int>((captured*4+kPPQ-1)/kPPQ)+1;p->stepCount=std::max(p->stepCount,need);engine_.publish(project_);if(arrangement_)arrangement_->repaint();
    }
    void syncChopControls(){
        suppressChopCallbacks_=true;auto*p=latestChopPattern();const Tick grid=p?p->chopQuantizeGridTicks:kPPQ/4;chopGridChoice_.setSelectedId(grid==kPPQ/2?1:(grid==kPPQ/8?3:2),juce::dontSendNotification);chopQuantizeStrength_.setValue(p?std::round(p->chopQuantizeStrength*100.0f):0.0,juce::dontSendNotification);chopHumanizeStrength_.setValue(p?std::round(p->chopHumanize*100.0f):0.0,juce::dontSendNotification);const bool enabled=p&&!p->chopEvents.empty()&&!recordingChops_;chopGridChoice_.setEnabled(enabled);chopQuantizeStrength_.setEnabled(enabled);chopHumanizeStrength_.setEnabled(enabled);chopReset_.setEnabled(enabled);suppressChopCallbacks_=false;
    }
    void beginChopGesture(){if(suppressChopCallbacks_||chopGestureActive_||recordingChops_)return;auto*p=latestChopPattern();if(!p||p->chopEvents.empty())return;chopBefore_=project_;chopGestureActive_=true;}
    void applyChopControls(){
        if(suppressChopCallbacks_||recordingChops_)return;auto*p=latestChopPattern();if(!p||p->chopEvents.empty())return;if(!chopGestureActive_)beginChopGesture();if(!chopGestureActive_)return;quantizeChopEvents(*p,p->chopQuantizeGridTicks,static_cast<float>(chopQuantizeStrength_.getValue()/100.0));humanizeChopEvents(*p,static_cast<float>(chopHumanizeStrength_.getValue()/100.0));engine_.publish(project_);if(arrangement_)arrangement_->repaint();
    }
    void endChopGesture(){if(!chopGestureActive_)return;chopGestureActive_=false;undo_.commit(std::move(chopBefore_),project_,"Edit chop feel");syncChopControls();status_.setText("Chop feel committed",juce::dontSendNotification);}
    void changeChopGrid(){
        if(suppressChopCallbacks_||recordingChops_)return;auto*p=latestChopPattern();if(!p||p->chopEvents.empty())return;const int id=chopGridChoice_.getSelectedId();const Tick grid=id==1?kPPQ/2:(id==3?kPPQ/8:kPPQ/4);if(grid==p->chopQuantizeGridTicks)return;Project before=project_;quantizeChopEvents(*p,grid,p->chopQuantizeStrength);undo_.commit(std::move(before),project_,"Change chop quantize grid");publishEdit(id==1?"Chop grid 1/8":(id==3?"Chop grid 1/32":"Chop grid 1/16"));
    }
    void resetLatestChops(){if(recordingChops_){status_.setText("Stop REC CHOPS before resetting feel",juce::dontSendNotification);return;}auto*p=latestChopPattern();if(!p){status_.setText("Record a chop take first",juce::dontSendNotification);return;}Project before=project_;resetChopEditing(*p);undo_.commit(std::move(before),project_,"Reset chop feel");publishEdit("Chop feel reset");syncChopControls();}
    Track*recordingTrack(){
        const int selected=selectedTrackIndex();if(selected>=0)return&project_.tracks[static_cast<std::size_t>(selected)];for(auto&t:project_.tracks)if(t.armed)return&t;for(auto&t:project_.tracks)if(t.name=="VOCAL")return&t;Track t;t.name="VOCAL";t.armed=true;project_.tracks.push_back(std::move(t));return&project_.tracks.back();
    }
    void toggleAudioRecording(){audioRecording_?finishAudioRecording():startAudioRecording();}
    void startAudioRecording(){
        if(!engine_.inputAvailable()){status_.setText("No audio input device; select/connect a microphone",juce::dontSendNotification);return;}audioRecordBefore_=project_;auto*t=recordingTrack();if(!t)return;t->armed=true;audioRecordTrackId_=t->id;audioRecordStartTick_=playheadTick();engine_.setInputMonitoring(t->inputMonitor);const auto maxFrames=static_cast<SampleIndex>(std::max(1,engine_.sampleRate()))*60*10;if(!engine_.beginRecording(maxFrames)){project_=std::move(audioRecordBefore_);publishEdit("Could not arm audio recorder");return;}audioRecording_=true;if(!engine_.isPlaying())engine_.play();status_.setText("REC AUDIO - "+juce::String(t->name),juce::dontSendNotification);
    }
    void finishAudioRecording(){
        if(!audioRecording_)return;auto captured=engine_.finishRecording();audioRecording_=false;if(captured.frames()<32){project_=std::move(audioRecordBefore_);publishEdit("Empty audio take discarded");return;}auto*t=project_.findTrack(audioRecordTrackId_);if(!t){project_=std::move(audioRecordBefore_);publishEdit("Recording track disappeared; take discarded");return;}
        SampleAsset asset;asset.name="Audio Take "+std::to_string(t->takes.size()+1);asset.audio=std::make_shared<AudioBuffer>(captured);std::filesystem::path base;if(projectPath_.has_parent_path())base=projectPath_.parent_path();else base=std::filesystem::path(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName().toStdString())/"FLOWDAW";auto dir=base/"recordings";std::filesystem::create_directories(dir);asset.path=std::filesystem::absolute(dir/("take_"+std::to_string(asset.id)+".wav"));WavFile::writeFloat32(asset.path,captured);const Id sampleId=asset.id;project_.samples.push_back(std::move(asset));t=project_.findTrack(audioRecordTrackId_);RecordingTake take;take.name="Take "+std::to_string(t->takes.size()+1);take.sampleId=sampleId;take.startTick=audioRecordStartTick_;take.lengthTicks=MusicalTime::samplesToTicks(captured.frames(),project_.transport.bpm,captured.sampleRate);t->takes.push_back(take);t->activeTakeId=take.id;undo_.commit(std::move(audioRecordBefore_),project_,"Record audio take");publishEdit("Recorded "+juce::String(take.name)+" ("+juce::String(static_cast<juce::int64>(captured.frames()))+" frames)");
    }
    void toggleInputMonitor(){
        auto*t=recordingTrack();if(!t)return;Project before=project_;t->inputMonitor=!t->inputMonitor;engine_.setInputMonitoring(t->inputMonitor);undo_.commit(std::move(before),project_,"Toggle input monitoring");publishEdit(t->inputMonitor?"Input monitoring ON":"Input monitoring OFF");
    }
    void cycleTake(int delta){
        const int index=selectedTrackIndex();if(index<0)return;auto&t=project_.tracks[static_cast<std::size_t>(index)];if(t.takes.empty()){status_.setText("Selected track has no recorded takes",juce::dontSendNotification);return;}int current=0;for(int i=0;i<static_cast<int>(t.takes.size());++i)if(t.takes[static_cast<std::size_t>(i)].id==t.activeTakeId)current=i;current=(current+delta+static_cast<int>(t.takes.size()))%static_cast<int>(t.takes.size());Project before=project_;t.activeTakeId=t.takes[static_cast<std::size_t>(current)].id;const auto name=t.takes[static_cast<std::size_t>(current)].name;undo_.commit(std::move(before),project_,"Select recording take");publishEdit("Active comp: "+juce::String(name));
    }
    void syncTransportVisualState(){const bool playing=engine_.isPlaying();if(play_.getToggleState()!=playing)play_.setToggleState(playing,juce::dontSendNotification);play_.setButtonText(playing?"Pause":"Play");}
    void togglePlayback(){if(engine_.isPlaying()){engine_.pause();status_.setText("Paused",juce::dontSendNotification);}else{engine_.play();status_.setText("Playing through JUCE device",juce::dontSendNotification);}syncTransportVisualState();}
    void applyNewProject(Project next,const juce::String&label,const juce::String&message){
        if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();pluginWindow_.reset();engine_.stop();project_=std::move(next);projectPath_.clear();settings_.lastProjectPath.clear();undo_=UndoStack{};engine_.publish(project_);
        refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshRackControls();syncChopControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();
        recoveredAtStartup_=false;preserveRecoveryOnExit_=false;resetRecoverySnapshot();projectLabel_.setText(label,juce::dontSendNotification);status_.setText(message,juce::dontSendNotification);saveDeviceSettings();
    }
    void showNewProjectMenu(){
        juce::PopupMenu menu;menu.addItem(1,"Blank Project — 120 BPM");menu.addItem(2,"Boom Bap Starter — 90 BPM");menu.addItem(3,"Trap Starter — 140 BPM");menu.addItem(4,"Lo-Fi Starter — 82 BPM");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&newProject_),[this](int result){
            if(result==1)applyNewProject(makeBlankProject(),"Blank Project • 120 BPM","New blank project");
            else if(result==2)applyNewProject(makeBeatTemplate("Boom Bap Starter",90.0,0.12f),"Boom Bap Starter • 90 BPM","Boom Bap template ready");
            else if(result==3)applyNewProject(makeBeatTemplate("Trap Starter",140.0,0.02f),"Trap Starter • 140 BPM","Trap template ready");
            else if(result==4)applyNewProject(makeBeatTemplate("Lo-Fi Starter",82.0,0.18f),"Lo-Fi Starter • 82 BPM","Lo-Fi template ready");
        });
    }
    void showFirstRunOnboarding(){
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Welcome to FLOWDAW","Start with New / Template, add folders in the Sample Browser, drag WAVs into the workspace, and use Ctrl/Cmd+K for the command palette. FLOWDAW autosaves recovery snapshots automatically.","Start making music");
    }
    void runWorkflowCommand(int id){
        switch(id){case 1:showNewProjectMenu();break;case 2:chooseProject();break;case 3:saveProject();break;case 4:chooseWav();break;case 5:togglePlayback();break;case 6:undoEdit();break;case 7:redoEdit();break;case 11:setEditorMode(EditorMode::Arrangement);break;case 12:setEditorMode(EditorMode::Piano);break;case 13:setEditorMode(EditorMode::Step);break;case 14:setEditorMode(EditorMode::Automation);break;case 15:setEditorMode(EditorMode::Sampler);break;default:break;}
    }
    void showCommandPalette(){
        juce::PopupMenu menu;menu.addSectionHeader("FLOWDAW Commands");menu.addItem(1,"New / Template    Ctrl/Cmd+N");menu.addItem(2,"Open Project    Ctrl/Cmd+O");menu.addItem(3,"Save Project    Ctrl/Cmd+S");menu.addItem(4,"Import WAV    Ctrl/Cmd+I");menu.addSeparator();menu.addItem(5,"Play / Pause    Ctrl/Cmd+Space");menu.addItem(6,"Undo    Ctrl/Cmd+Z");menu.addItem(7,"Redo    Ctrl/Cmd+Shift+Z");menu.addSeparator();menu.addItem(11,"Arrangement    Ctrl/Cmd+1");menu.addItem(12,"Piano Roll    Ctrl/Cmd+2");menu.addItem(13,"Sequencer    Ctrl/Cmd+3");menu.addItem(14,"Automation    Ctrl/Cmd+4");menu.addItem(15,"Sampler    Ctrl/Cmd+5");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&commandPalette_),[this](int result){runWorkflowCommand(result);});
    }
    bool keyPressed(const juce::KeyPress&key,juce::Component*)override{
        const auto mods=key.getModifiers();if(!mods.isCommandDown())return false;const int code=key.getKeyCode();
        if(code=='K'){showCommandPalette();return true;}if(code=='N'){showNewProjectMenu();return true;}if(code=='O'){chooseProject();return true;}if(code=='S'){saveProject();return true;}if(code=='I'){chooseWav();return true;}
        if(code=='Z'){mods.isShiftDown()?redoEdit():undoEdit();return true;}if(code==juce::KeyPress::spaceKey){togglePlayback();return true;}
        if(code>='1'&&code<='5'){runWorkflowCommand(11+(code-'1'));return true;}return false;
    }
    void installShortcutListeners(juce::Component&component){component.addKeyListener(this);for(auto*child:component.getChildren())if(child)installShortcutListeners(*child);}

    void refreshRackTargets(){
        const int previous=rackTargetChoice_.getSelectedId();rackTargetChoice_.clear(juce::dontSendNotification);rackTargetChoice_.addItem("MASTER • Rack",1);
        const int trackIndex=selectedTrackIndex();rackTargetChoice_.addItem(trackIndex>=0?"TRACK • "+juce::String(project_.tracks[static_cast<std::size_t>(trackIndex)].name)+" • Rack":"TRACK • none • Rack",2);
        int id=100;for(auto const&bus:project_.buses)rackTargetChoice_.addItem("BUS • "+juce::String(bus.name)+" • Rack",id++);
        const bool previousValid=previous==1||previous==2||(previous>=100&&previous<100+static_cast<int>(project_.buses.size()));rackTargetChoice_.setSelectedId(previousValid?previous:1,juce::dontSendNotification);
    }
    int rackTargetForMixer()const{
        const int target=mixerTargetChoice_.getSelectedId();if(target==2)return 1;if(target==1)return 2;if(target>=100)return target;return 1;
    }
    int mixerTargetForRack()const{
        const int target=rackTargetChoice_.getSelectedId();if(target==1)return 2;if(target==2)return 1;if(target>=100)return target;return 1;
    }
    void syncRackTargetToMixer(){
        suppressRackCallbacks_=true;refreshRackTargets();const int target=rackTargetForMixer();if(rackTargetChoice_.getSelectedId()!=target)rackTargetChoice_.setSelectedId(target,juce::dontSendNotification);suppressRackCallbacks_=false;
    }
    void syncMixerTargetFromRack(){
        const int target=mixerTargetForRack();suppressMixerCallbacks_=true;refreshMixerTargets();mixerTargetChoice_.setSelectedId(target,juce::dontSendNotification);suppressMixerCallbacks_=false;syncMixerControls();refreshMixerRoutingControls();
    }
    int selectedRackBusIndex()const{const int id=rackTargetChoice_.getSelectedId();const int index=id>=100?id-100:-1;return index>=0&&index<static_cast<int>(project_.buses.size())?index:-1;}
    std::vector<PluginInstance>*rackPlugins(){
        const int target=rackTargetChoice_.getSelectedId();if(target==2){const int index=selectedTrackIndex();if(index<0)return nullptr;return&project_.tracks[static_cast<std::size_t>(index)].mixer.plugins;}
        if(target>=100){const int index=selectedRackBusIndex();if(index<0)return nullptr;return&project_.buses[static_cast<std::size_t>(index)].mixer.plugins;}return&project_.master.plugins;
    }
    const std::vector<PluginInstance>*rackPlugins()const{
        const int target=rackTargetChoice_.getSelectedId();if(target==2){const int index=selectedTrackIndex();if(index<0)return nullptr;return&project_.tracks[static_cast<std::size_t>(index)].mixer.plugins;}
        if(target>=100){const int index=selectedRackBusIndex();if(index<0)return nullptr;return&project_.buses[static_cast<std::size_t>(index)].mixer.plugins;}return&project_.master.plugins;
    }
    PluginInstance*selectedRackPlugin(){auto*rack=rackPlugins();const int index=rackPluginChoice_.getSelectedId()-1;return rack&&index>=0&&index<static_cast<int>(rack->size())?&(*rack)[static_cast<std::size_t>(index)]:nullptr;}
    const PluginInstance*selectedRackPlugin()const{auto*rack=rackPlugins();const int index=rackPluginChoice_.getSelectedId()-1;return rack&&index>=0&&index<static_cast<int>(rack->size())?&(*rack)[static_cast<std::size_t>(index)]:nullptr;}
    void refreshRackControls(){
        suppressRackCallbacks_=true;refreshRackTargets();const int previous=rackPluginChoice_.getSelectedId();rackPluginChoice_.clear(juce::dontSendNotification);auto*rack=rackPlugins();if(rack){int id=1;for(auto const&p:*rack){auto label=juce::String(id)+" • "+juce::String(p.name)+" ["+juce::String(p.format)+"] • "+(!p.enabled?"DISABLED":(p.bypass?"BYPASS":"ACTIVE"));rackPluginChoice_.addItem(label,id++);}if(!rack->empty())rackPluginChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(rack->size())?previous:1,juce::dontSendNotification);}rackPluginChoice_.setTextWhenNothingSelected(rack&&rack->empty()?"Rack empty":"Select insert");suppressRackCallbacks_=false;syncRackControls();
    }
    void syncRackControls(){
        suppressRackCallbacks_=true;auto*p=selectedRackPlugin();auto*rack=rackPlugins();const int index=rackPluginChoice_.getSelectedId()-1;rackWet_.setEnabled(p!=nullptr);rackEnabled_.setEnabled(p!=nullptr);rackBypass_.setEnabled(p!=nullptr);rackRemove_.setEnabled(p!=nullptr);rackMoveUp_.setEnabled(p&&index>0);rackMoveDown_.setEnabled(p&&rack&&index+1<static_cast<int>(rack->size()));addRackExternal_.setEnabled(rack!=nullptr);openRackEditor_.setEnabled(p&&p->format!="builtin");
        rackWet_.setValue(p?p->wet:1.0f,juce::dontSendNotification);rackEnabled_.setButtonText(p&&p->enabled?"Disable":"Enable");rackBypass_.setButtonText(p&&p->bypass?"Activate":"Bypass");rackParam_.setEnabled(false);rackParamLabel_.setText("Parameter",juce::dontSendNotification);rackParam_.setTextValueSuffix({});
        if(p&&p->format=="builtin"){
            if(p->identifier=="flow.gain"){rackParam_.setRange(0.0,4.0,0.01);rackParam_.setValue(pluginParameterValue(*p,"gain",1.0f),juce::dontSendNotification);rackParam_.setTextValueSuffix(" gain");rackParamLabel_.setText("Gain",juce::dontSendNotification);rackParam_.setEnabled(true);}
            else if(p->identifier=="flow.softclip"){rackParam_.setRange(0.0,1.0,0.01);rackParam_.setValue(pluginParameterValue(*p,"drive",0.25f),juce::dontSendNotification);rackParam_.setTextValueSuffix(" drive");rackParamLabel_.setText("Drive",juce::dontSendNotification);rackParam_.setEnabled(true);}
            else if(p->identifier=="flow.width"){rackParam_.setRange(0.0,2.0,0.01);rackParam_.setValue(pluginParameterValue(*p,"width",1.0f),juce::dontSendNotification);rackParam_.setTextValueSuffix(" width");rackParamLabel_.setText("Width",juce::dontSendNotification);rackParam_.setEnabled(true);}
        }else if(p){rackParamLabel_.setText("External state",juce::dontSendNotification);}
        suppressRackCallbacks_=false;
    }
    PluginInstance*findPluginInstance(Id id){
        for(auto&p:project_.master.plugins)if(p.id==id)return&p;for(auto&t:project_.tracks){for(auto&p:t.mixer.plugins)if(p.id==id)return&p;if(t.externalInstrumentEnabled&&t.externalInstrument.id==id)return&t.externalInstrument;}for(auto&b:project_.buses)for(auto&p:b.mixer.plugins)if(p.id==id)return&p;return nullptr;
    }
    void finishRackEditorSession(){
        if(rackEditorPluginId_==0)return;const bool dirty=rackEditorDirty_;rackEditorPluginId_=0;rackEditorDirty_=false;if(dirty){undo_.commit(std::move(rackEditorBefore_),project_,"Edit external plugin state");publishEdit("External plugin state committed");}
    }
    void openRackEditor(){
        auto*p=selectedRackPlugin();if(!p){status_.setText("Select a rack insert first",juce::dontSendNotification);return;}if(p->format=="builtin"){status_.setText("FLOW native parameters are edited directly in the rack",juce::dontSendNotification);return;}if(pluginWindow_&&pluginWindow_->isVisible()){status_.setText("Close the current plugin editor first",juce::dontSendNotification);return;}if(safety_.isQuarantined(p->identifier)){status_.setText("Plugin is quarantined; clear it with FLOWDAW Doctor before retrying",juce::dontSendNotification);return;}
        PluginDescriptor descriptor;descriptor.format=p->format;descriptor.identifier=p->identifier;descriptor.name=p->name;auto*fmt=formatFor(descriptor);if(!fmt){status_.setText("No JUCE backend for this insert",juce::dontSendNotification);return;}juce::OwnedArray<juce::PluginDescription>desc;fmt->findAllTypesForFile(desc,juce::String(p->identifier));if(desc.isEmpty()){safety_.noteFailure(p->identifier,"Rack editor could not recreate plugin description");saveSafety();status_.setText("Could not recreate rack plugin",juce::dontSendNotification);return;}
        const Id pluginId=p->id;const std::string initialState=p->opaqueState;const std::string identifier=p->identifier;rackEditorBefore_=project_;rackEditorPluginId_=pluginId;rackEditorDirty_=false;auto setup=deviceManager_.getAudioDeviceSetup();formatManager_.createPluginInstanceAsync(*desc[0],setup.sampleRate>0?setup.sampleRate:48000.0,setup.bufferSize>0?setup.bufferSize:256,[this,pluginId,initialState,identifier](std::unique_ptr<juce::AudioPluginInstance>instance,const juce::String&error){
            if(!instance){rackEditorPluginId_=0;safety_.noteFailure(identifier,error.toStdString());saveSafety();status_.setText("Plugin load failed: "+error,juce::dontSendNotification);return;}if(!initialState.empty()){auto state=decodePluginStateHex(initialState);if(state.getSize())instance->setStateInformation(state.getData(),static_cast<int>(state.getSize()));}safety_.noteSuccess(identifier);saveSafety();
            pluginWindow_=std::make_unique<PluginEditorWindow>(std::move(instance),[this,pluginId](const std::string&state){auto*slot=findPluginInstance(pluginId);if(!slot||slot->opaqueState==state)return;slot->opaqueState=state;rackEditorDirty_=true;engine_.publish(project_);},[this]{finishRackEditorSession();});status_.setText("Editing live rack insert; state auto-syncs",juce::dontSendNotification);
        });
    }
    void addBuiltinToRack(const std::string&id){
        auto*rack=rackPlugins();if(!rack){status_.setText("Select a valid Track or Bus rack",juce::dontSendNotification);return;}Project before=project_;rack->push_back(makeBuiltinPlugin(id));const int selected=static_cast<int>(rack->size());const auto name=rack->back().name;undo_.commit(std::move(before),project_,"Add rack plugin");publishEdit("Added "+juce::String(name));rackPluginChoice_.setSelectedId(selected,juce::dontSendNotification);syncRackControls();
    }
    void addSelectedEffectToRack(){
        const int pluginIndex=selectedPluginIndex();if(pluginIndex<0||pluginIndex>=static_cast<int>(plugins_.size())){status_.setText("Scan and select an effect first",juce::dontSendNotification);return;}const auto&descriptor=plugins_[static_cast<std::size_t>(pluginIndex)];if(descriptor.instrument){status_.setText("Selected plugin is an instrument, not an effect insert",juce::dontSendNotification);return;}auto*rack=rackPlugins();if(!rack){status_.setText("Select a valid Track or Bus rack",juce::dontSendNotification);return;}
        PluginInstance instance;instance.format=descriptor.format;instance.identifier=descriptor.identifier;instance.name=descriptor.name;instance.enabled=true;instance.bypass=false;instance.wet=1.0f;Project before=project_;rack->push_back(std::move(instance));const int selected=static_cast<int>(rack->size());undo_.commit(std::move(before),project_,"Add rack plugin");publishEdit("Added "+juce::String(descriptor.name));rackPluginChoice_.setSelectedId(selected,juce::dontSendNotification);syncRackControls();
    }
    void moveRackPlugin(int delta){
        auto*rack=rackPlugins();const int index=rackPluginChoice_.getSelectedId()-1;if(!rack||index<0||index>=static_cast<int>(rack->size())){status_.setText("Select a rack insert first",juce::dontSendNotification);return;}const int target=index+delta;if(target<0||target>=static_cast<int>(rack->size()))return;Project before=project_;std::swap((*rack)[static_cast<std::size_t>(index)],(*rack)[static_cast<std::size_t>(target)]);undo_.commit(std::move(before),project_,"Reorder rack plugin");publishEdit("Reordered rack insert");rackPluginChoice_.setSelectedId(target+1,juce::dontSendNotification);syncRackControls();
    }
    void toggleRackEnabled(){
        auto*p=selectedRackPlugin();if(!p){status_.setText("Select a rack insert first",juce::dontSendNotification);return;}Project before=project_;p->enabled=!p->enabled;const auto name=p->name;const bool enabled=p->enabled;undo_.commit(std::move(before),project_,"Toggle plugin enabled");publishEdit(juce::String(name)+(enabled?" enabled":" disabled"));
    }
    void toggleRackBypass(){
        auto*p=selectedRackPlugin();if(!p){status_.setText("Select a rack insert first",juce::dontSendNotification);return;}Project before=project_;p->bypass=!p->bypass;const auto name=p->name;const bool bypass=p->bypass;undo_.commit(std::move(before),project_,"Toggle plugin bypass");publishEdit(juce::String(name)+(bypass?" bypassed":" active"));
    }
    void removeRackPlugin(){
        auto*rack=rackPlugins();int index=rackPluginChoice_.getSelectedId()-1;if(!rack||index<0||index>=static_cast<int>(rack->size())){status_.setText("Select a rack insert first",juce::dontSendNotification);return;}const Id removingId=(*rack)[static_cast<std::size_t>(index)].id;if(removingId==rackEditorPluginId_)pluginWindow_.reset();rack=rackPlugins();index=rackPluginChoice_.getSelectedId()-1;if(!rack||index<0||index>=static_cast<int>(rack->size()))return;Project before=project_;const auto name=(*rack)[static_cast<std::size_t>(index)].name;rack->erase(rack->begin()+index);undo_.commit(std::move(before),project_,"Remove rack plugin");publishEdit("Removed "+juce::String(name));
    }
    void beginRackGesture(){if(suppressRackCallbacks_||rackGestureActive_||!selectedRackPlugin())return;rackBefore_=project_;rackGestureActive_=true;}
    void applyRackSliders(){
        if(suppressRackCallbacks_)return;auto*p=selectedRackPlugin();if(!p)return;if(!rackGestureActive_)beginRackGesture();p->wet=static_cast<float>(rackWet_.getValue());
        if(p->format=="builtin"){if(p->identifier=="flow.gain")setPluginParameter(*p,"gain",static_cast<float>(rackParam_.getValue()));else if(p->identifier=="flow.softclip")setPluginParameter(*p,"drive",static_cast<float>(rackParam_.getValue()));else if(p->identifier=="flow.width")setPluginParameter(*p,"width",static_cast<float>(rackParam_.getValue()));}
        engine_.publish(project_);
    }
    void endRackGesture(){if(!rackGestureActive_)return;rackGestureActive_=false;undo_.commit(std::move(rackBefore_),project_,"Edit rack plugin");refreshRackControls();status_.setText("Rack edit committed",juce::dontSendNotification);}
    void chooseWav(){
        chooser_=std::make_unique<juce::FileChooser>("Import WAV",juce::File::getSpecialLocation(juce::File::userMusicDirectory),"*.wav");
        chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser&fc){auto f=fc.getResult();if(f.existsAsFile())importWavFile(std::filesystem::path(f.getFullPathName().toStdString()));chooser_.reset();});
    }
    bool isInterestedInDragSource(const SourceDetails& details) override {
        const auto description=details.description.toString();
        if(!description.startsWith("flowdaw-sample:")) return false;
        const auto pathText=description.substring(15);
        return pathText.isNotEmpty() && pathText.endsWithIgnoreCase(".wav");
    }
    void itemDropped(const SourceDetails& details) override {
        if(!isInterestedInDragSource(details)) return;
        const auto description=details.description.toString();
        const auto path=std::filesystem::path(description.substring(15).toStdString());
        std::error_code ec;
        if(!std::filesystem::is_regular_file(path,ec)) return;
        importWavFile(path);
    }
    void importWavFile(const std::filesystem::path&path){
        try{
            auto audio=std::make_shared<AudioBuffer>(WavFile::read(path));Project before=project_;SampleAsset sample;sample.path=std::filesystem::absolute(path);sample.name=path.filename().string();sample.audio=audio;const Id sampleId=sample.id;project_.samples.push_back(std::move(sample));
            int trackIndex=selectedTrackIndex();if(trackIndex<0){Track t;t.name="Sample";project_.tracks.push_back(std::move(t));trackIndex=static_cast<int>(project_.tracks.size())-1;}
            auto&track=project_.tracks[static_cast<std::size_t>(trackIndex)];Clip clip;clip.sampleId=sampleId;clip.sourceLength=audio->frames();clip.lengthTicks=MusicalTime::samplesToTicks(audio->frames(),project_.transport.bpm,audio->sampleRate);track.clips.push_back(clip);
            undo_.commit(std::move(before),project_,"Import WAV");publishEdit("Imported "+juce::String(path.filename().string()));selectSampleId(sampleId);setEditorMode(EditorMode::Sampler);
        }catch(const std::exception&e){status_.setText("Import failed: "+juce::String(e.what()),juce::dontSendNotification);}
    }
    void analyzeSelectedSample(){
        auto*smp=selectedSample();if(!smp||!smp->audio){status_.setText("Select an audio sample first",juce::dontSendNotification);return;}
        Project before=project_;const auto grid=estimateBeatGrid(*smp->audio,55,210);if(grid.valid()){smp->detectedBpm=grid.bpm;smp->bpmConfidence=static_cast<float>(grid.confidence);}else{const auto est=detectBpm(*smp->audio,55,210);smp->detectedBpm=est.bpm;smp->bpmConfidence=static_cast<float>(est.confidence);}
        undo_.commit(std::move(before),project_,"Analyze sample");publishEdit(smp->detectedBpm>0?"Detected "+juce::String(smp->detectedBpm,1)+" BPM":"BPM not confidently detected");
    }
    bool canReplaceSelectedSlices(SampleAsset*smp){
        if(!smp||!smp->audio){status_.setText("Select an audio sample first",juce::dontSendNotification);return false;}
        if(sampleReferencedByChops(smp->id)){status_.setText("Slice map is used by recorded chops; edit markers instead",juce::dontSendNotification);return false;}return true;
    }
    void applySliceRanges(SampleAsset&smp,const std::vector<SliceRange>&ranges,const std::string&prefix,const std::string&undoName){
        Project before=project_;smp.slices.clear();for(std::size_t i=0;i<ranges.size();++i){SampleSlice sl;sl.name=prefix+" "+std::to_string(i+1);sl.startFrame=ranges[i].startFrame;sl.endFrame=ranges[i].endFrame;smp.slices.push_back(std::move(sl));}
        undo_.commit(std::move(before),project_,undoName);publishEdit(juce::String(undoName)+": "+juce::String(static_cast<int>(smp.slices.size()))+" slices");
    }
    void equalChopSelected(int count){auto*smp=selectedSample();if(!canReplaceSelectedSlices(smp))return;applySliceRanges(*smp,makeEqualSlices(*smp->audio,count),"Slice","Equal chop");}
    void autoChopSelected(){
        auto*smp=selectedSample();if(!canReplaceSelectedSlices(smp))return;auto tr=detectTransients(*smp->audio,.9,.08);std::vector<SampleIndex>b{0};for(auto frame:tr)if(frame>256&&frame<smp->audio->frames()-256&&b.size()<64)b.push_back(frame);b.push_back(smp->audio->frames());std::sort(b.begin(),b.end());b.erase(std::unique(b.begin(),b.end()),b.end());if(b.size()<3){status_.setText("Too few transients; try CHOP 8",juce::dontSendNotification);return;}std::vector<SliceRange>ranges;for(std::size_t i=0;i+1<b.size()&&i<64;++i)ranges.push_back({b[i],b[i+1]});applySliceRanges(*smp,ranges,"Chop","Auto chop");
    }
    void beatGridChopSelected(int beatsPerSlice){
        auto*smp=selectedSample();if(!canReplaceSelectedSlices(smp))return;const auto grid=estimateBeatGrid(*smp->audio,55,210);if(!grid.valid()){status_.setText("Beat grid not confident enough",juce::dontSendNotification);return;}const auto ranges=makeBeatSlices(*smp->audio,grid,beatsPerSlice);if(ranges.empty()){status_.setText("Could not build beat slices",juce::dontSendNotification);return;}smp->detectedBpm=grid.bpm;smp->bpmConfidence=static_cast<float>(grid.confidence);applySliceRanges(*smp,ranges,beatsPerSlice==1?"Beat":"Bar",beatsPerSlice==1?"Chop by beat":"Chop by bar");
    }
    void matchSelectedSampleBpm(){
        auto*smp=selectedSample();if(!smp||!smp->audio){status_.setText("Select an audio sample first",juce::dontSendNotification);return;}auto*clip=firstClipForSample(smp->id);if(!clip){status_.setText("Selected sample is not placed in Arrangement",juce::dontSendNotification);return;}
        double source=smp->detectedBpm;if(source<=0){const auto est=detectBpm(*smp->audio,55,210);if(!est.valid()){status_.setText("Analyze BPM first",juce::dontSendNotification);return;}source=est.bpm;}
        const double ratio=source/project_.transport.bpm;if(ratio<.5||ratio>2.0){status_.setText("Match ratio outside 0.5x-2x",juce::dontSendNotification);return;}if(std::abs(ratio-1.0)<.003){status_.setText("Sample already matches project BPM",juce::dontSendNotification);return;}
        Project before=project_;const Id sourceId=smp->id;const std::string sourceName=smp->name;const float confidence=smp->bpmConfidence;const auto sourceSlices=smp->slices;auto stretched=std::make_shared<AudioBuffer>(matchBpmWsola(*smp->audio,source,project_.transport.bpm));SampleAsset derived;derived.name=sourceName+" @ "+std::to_string(static_cast<int>(std::llround(project_.transport.bpm)))+" BPM";derived.sourceSampleId=sourceId;derived.timeRatio=ratio;derived.detectedBpm=project_.transport.bpm;derived.bpmConfidence=confidence;derived.audio=stretched;
        for(auto const&old:sourceSlices){SampleSlice sl=old;sl.id=nextId();sl.startFrame=static_cast<SampleIndex>(std::llround(old.startFrame*ratio));sl.endFrame=std::min<SampleIndex>(stretched->frames(),static_cast<SampleIndex>(std::llround(old.endFrame*ratio)));derived.slices.push_back(std::move(sl));}
        const Id derivedId=derived.id;project_.samples.push_back(std::move(derived));clip=firstClipForSample(sourceId);clip->sampleId=derivedId;clip->sourceStart=0;clip->sourceLength=stretched->frames();clip->lengthTicks=MusicalTime::samplesToTicks(stretched->frames(),project_.transport.bpm,stretched->sampleRate);undo_.commit(std::move(before),project_,"Match sample BPM");publishEdit("Matched "+juce::String(source,1)+" -> "+juce::String(project_.transport.bpm,1)+" BPM");selectSampleId(derivedId);
    }
    void chooseMixExport(){
        auto base=projectPath_.empty()?juce::File::getSpecialLocation(juce::File::userDocumentsDirectory):juce::File(projectPath_.parent_path().string());auto name=projectPath_.empty()?"FLOWDAW_mix.wav":juce::String(projectPath_.stem().string()+"_mix.wav");chooser_=std::make_unique<juce::FileChooser>("Export mix",base.getChildFile(name),"*.wav");
        chooser_->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser&fc){auto f=fc.getResult();if(f!=juce::File{})try{exportProjectWav(project_,std::filesystem::path(f.getFullPathName().toStdString()),2.0,pluginHost_);status_.setText("Exported mix: "+f.getFileName(),juce::dontSendNotification);}catch(const std::exception&e){status_.setText("Export failed: "+juce::String(e.what()),juce::dontSendNotification);}chooser_.reset();});
    }
    void chooseStemExport(){
        auto base=projectPath_.empty()?juce::File::getSpecialLocation(juce::File::userDocumentsDirectory):juce::File(projectPath_.parent_path().string());chooser_=std::make_unique<juce::FileChooser>("Export stems to folder",base.getChildFile("stems"));
        chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[this](const juce::FileChooser&fc){auto dir=fc.getResult();if(dir!=juce::File{})try{const auto paths=exportTrackStems(project_,std::filesystem::path(dir.getFullPathName().toStdString()),2.0,pluginHost_);status_.setText("Exported "+juce::String(static_cast<int>(paths.size()))+" stems",juce::dontSendNotification);}catch(const std::exception&e){status_.setText("Stem export failed: "+juce::String(e.what()),juce::dontSendNotification);}chooser_.reset();});
    }
    void chooseProject(){chooser_=std::make_unique<juce::FileChooser>("Open FLOWDAW project",juce::File(projectPath_.string()),"*.flow");chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser&fc){auto f=fc.getResult();if(f.existsAsFile())loadProjectFile(std::filesystem::path(f.getFullPathName().toStdString()));chooser_.reset();});}
    void loadProjectFile(const std::filesystem::path&p){try{if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();pluginWindow_.reset();engine_.stop();project_=ProjectSerializer::load(p,true);projectPath_=p;settings_.lastProjectPath=p;undo_=UndoStack{};engine_.publish(project_);refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshRackControls();syncChopControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();projectLabel_.setText("Project: "+juce::String(p.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);recoveredAtStartup_=false;preserveRecoveryOnExit_=false;resetRecoverySnapshot();status_.setText("Project loaded; recovery snapshot armed",juce::dontSendNotification);}catch(const std::exception&e){status_.setText("Open failed: "+juce::String(e.what()),juce::dontSendNotification);}}
    std::vector<std::filesystem::path> pluginRoots()const{
        if(!settings_.pluginRoots.empty())return settings_.pluginRoots;std::vector<std::filesystem::path> roots;auto home=std::filesystem::path(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName().toStdString());roots.push_back(home/".vst3");roots.push_back("/usr/lib/vst3");roots.push_back("/usr/local/lib/vst3");
#if JUCE_MAC
        roots.push_back(home/"Library/Audio/Plug-Ins/VST3");roots.push_back(home/"Library/Audio/Plug-Ins/Components");roots.push_back("/Library/Audio/Plug-Ins/VST3");roots.push_back("/Library/Audio/Plug-Ins/Components");
#endif
        return roots;
    }
    static std::string lowerPluginText(std::string s){for(char&ch:s)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));return s;}
    int selectedPluginIndex()const{const int row=pluginChoice_.getSelectedId()-1;return row>=0&&row<static_cast<int>(visiblePluginIndices_.size())?visiblePluginIndices_[static_cast<std::size_t>(row)]:-1;}
    void refreshPluginChoice(){
        const int previousIndex=selectedPluginIndex();std::string previousIdentifier;if(previousIndex>=0&&previousIndex<static_cast<int>(plugins_.size()))previousIdentifier=plugins_[static_cast<std::size_t>(previousIndex)].identifier;
        const auto query=lowerPluginText(pluginSearch_.getText().trim().toStdString());const int kind=pluginKindChoice_.getSelectedId();
        visiblePluginIndices_.clear();pluginChoice_.clear(juce::dontSendNotification);int id=1,selectedId=0;
        for(std::size_t i=0;i<plugins_.size();++i){auto const&p=plugins_[i];if(kind==2&&!p.instrument)continue;if(kind==3&&p.instrument)continue;const auto haystack=lowerPluginText(p.name+" "+p.format+" "+p.identifier);if(!query.empty()&&haystack.find(query)==std::string::npos)continue;visiblePluginIndices_.push_back(static_cast<int>(i));auto label=p.name+" ["+p.format+(p.instrument?"/instrument":"/effect")+"]";pluginChoice_.addItem(juce::String(label),id);if(!previousIdentifier.empty()&&p.identifier==previousIdentifier)selectedId=id;++id;}
        if(!visiblePluginIndices_.empty())pluginChoice_.setSelectedId(selectedId>0?selectedId:1,juce::dontSendNotification);
        pluginChoice_.setTextWhenNothingSelected(plugins_.empty()?"Scan plugins first":"No plugin matches");
    }
    void scanPlugins(){std::string error;plugins_=scanPluginsWithJuce(pluginRoots(),error,&safety_);saveSafety();refreshPluginChoice();int quarantined=0;for(auto const&r:safety_.records())if(r.quarantined)++quarantined;auto base="JUCE scan: "+juce::String(static_cast<int>(plugins_.size()))+" plugin type(s), "+juce::String(static_cast<int>(visiblePluginIndices_.size()))+" shown, "+juce::String(quarantined)+" quarantined";status_.setText(error.empty()?base:base+" | "+juce::String(error),juce::dontSendNotification);}
    int selectedTrackIndex()const{const int index=trackChoice_.getSelectedId()-1;return index>=0&&index<static_cast<int>(project_.tracks.size())?index:-1;}
    void refreshTrackChoice(){
        const int previous=trackChoice_.getSelectedId();trackChoice_.clear(juce::dontSendNotification);int id=1;
        for(auto const&t:project_.tracks){std::string label=t.name;if(t.externalInstrumentEnabled&&!t.externalInstrument.name.empty())label+="  •  "+t.externalInstrument.name;trackChoice_.addItem(juce::String(label),id++);}
        if(!project_.tracks.empty())trackChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(project_.tracks.size())?previous:1,juce::dontSendNotification);
    }
    void publishEdit(const juce::String&message){engine_.publish(project_);refreshTrackChoice();refreshMixerTargets();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshMixerRoutingControls();syncRackTargetToMixer();refreshRackControls();syncChopControls();updateBpmLabel();if(arrangement_)arrangement_->projectChanged();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();status_.setText(message,juce::dontSendNotification);}
    enum class EditorMode{Arrangement,Piano,Step,Automation,Sampler};
    void setEditorMode(EditorMode mode){
        editorMode_=mode;const bool arrangement=mode==EditorMode::Arrangement,piano=mode==EditorMode::Piano,step=mode==EditorMode::Step,automation=mode==EditorMode::Automation,sampler=mode==EditorMode::Sampler;
        if(arrangement_)arrangement_->setVisible(arrangement);if(piano_)piano_->setVisible(piano);if(sequencer_)sequencer_->setVisible(step);if(automationAssist_)automationAssist_->setVisible(automation);if(sampler_)sampler_->setVisible(sampler);
        patternChoice_.setVisible(piano||step);sampleChoice_.setVisible(sampler);bankPrev_.setVisible(sampler);bankNext_.setVisible(sampler);
        arrangementTab_.setToggleState(arrangement,juce::dontSendNotification);pianoTab_.setToggleState(piano,juce::dontSendNotification);sequencerTab_.setToggleState(step,juce::dontSendNotification);automationTab_.setToggleState(automation,juce::dontSendNotification);samplerTab_.setToggleState(sampler,juce::dontSendNotification);
        note_.setText(arrangement?"ARRANGE • Timeline + Browser":(piano?"EDIT • Piano Roll":(step?"EDIT • Step Sequencer":(automation?"AUTOMATE • Lanes + Assist":"SAMPLE • Chops + Pads"))),juce::dontSendNotification);
        if(piano||step)syncPatternEditors();if(automation&&automationAssist_)automationAssist_->refresh();if(sampler)syncSamplerSample();
    }
    void refreshPatternChoice(){
        const int previous=patternChoice_.getSelectedId();patternChoice_.clear(juce::dontSendNotification);int id=1;
        for(auto const&p:project_.patterns){auto label=p.name+"  •  "+std::to_string(p.midiNotes.size())+" notes";patternChoice_.addItem(juce::String(label),id++);}
        if(!project_.patterns.empty())patternChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(project_.patterns.size())?previous:1,juce::dontSendNotification);else{if(piano_)piano_->setPatternId(0);if(sequencer_)sequencer_->setPatternId(0);}
        syncPatternEditors();
    }
    void refreshSampleChoice(){
        const int previous=sampleChoice_.getSelectedId();sampleChoice_.clear(juce::dontSendNotification);int id=1;
        for(auto const&s:project_.samples){auto label=s.name+"  •  "+std::to_string(s.slices.size())+" slices";sampleChoice_.addItem(juce::String(label),id++);}
        if(!project_.samples.empty())sampleChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(project_.samples.size())?previous:1,juce::dontSendNotification);else if(sampler_)sampler_->setSampleId(0);
        syncSamplerSample();
    }
    void syncPatternEditors(){const int index=patternChoice_.getSelectedId()-1;const Id id=index>=0&&index<static_cast<int>(project_.patterns.size())?project_.patterns[static_cast<std::size_t>(index)].id:0;if(piano_)piano_->setPatternId(id);if(sequencer_)sequencer_->setPatternId(id);}
    void syncSamplerSample(){const int index=sampleChoice_.getSelectedId()-1;if(sampler_)sampler_->setSampleId(index>=0&&index<static_cast<int>(project_.samples.size())?project_.samples[static_cast<std::size_t>(index)].id:0);}
    void updateBpmLabel(){bpmLabel_.setText(juce::String(project_.transport.bpm,1)+" BPM",juce::dontSendNotification);}
    void changeBpm(double delta){Project before=project_;project_.transport.bpm=std::clamp(project_.transport.bpm+delta,20.0,300.0);undo_.commit(std::move(before),project_,"Change BPM");publishEdit("BPM "+juce::String(project_.transport.bpm,1));}
    void refreshMixerTargets(){
        suppressMixerCallbacks_=true;const int previous=mixerTargetChoice_.getSelectedId();mixerTargetChoice_.clear(juce::dontSendNotification);
        const int trackIndex=selectedTrackIndex();mixerTargetChoice_.addItem(trackIndex>=0?"TRACK • "+juce::String(project_.tracks[static_cast<std::size_t>(trackIndex)].name):"TRACK • none",1);
        int id=100;for(auto const&bus:project_.buses)mixerTargetChoice_.addItem("BUS • "+juce::String(bus.name),id++);
        mixerTargetChoice_.addItem("MASTER",2);
        const bool valid=previous==1||previous==2||(previous>=100&&previous<100+static_cast<int>(project_.buses.size()));mixerTargetChoice_.setSelectedId(valid?previous:1,juce::dontSendNotification);suppressMixerCallbacks_=false;
    }
    int selectedMixerBusIndex()const{const int id=mixerTargetChoice_.getSelectedId();const int index=id>=100?id-100:-1;return index>=0&&index<static_cast<int>(project_.buses.size())?index:-1;}
    MixerChannel* selectedMixerChannel(){
        const int target=mixerTargetChoice_.getSelectedId();if(target==1){const int index=selectedTrackIndex();return index>=0?&project_.tracks[static_cast<std::size_t>(index)].mixer:nullptr;}
        if(target>=100){const int index=selectedMixerBusIndex();return index>=0?&project_.buses[static_cast<std::size_t>(index)].mixer:nullptr;}return nullptr;
    }
    juce::String activeMixerName()const{
        const int target=mixerTargetChoice_.getSelectedId();if(target==2)return"MASTER";
        if(target>=100){const int index=selectedMixerBusIndex();return index>=0?"BUS • "+juce::String(project_.buses[static_cast<std::size_t>(index)].name):"BUS";}
        const int index=selectedTrackIndex();return index>=0?"TRACK • "+juce::String(project_.tracks[static_cast<std::size_t>(index)].name):"TRACK • none";
    }
    void syncMixerControls(){
        suppressMixerCallbacks_=true;const bool master=mixerTargetChoice_.getSelectedId()==2;auto*channel=selectedMixerChannel();const bool valid=master||channel!=nullptr;
        mixerSectionLabel_.setText("MIXER • "+activeMixerName(),juce::dontSendNotification);mixerVolume_.setEnabled(valid);mixerPan_.setEnabled(channel!=nullptr);muteTrack_.setEnabled(channel!=nullptr);soloTrack_.setEnabled(channel!=nullptr);
        if(master){mixerVolume_.setValue(project_.master.volume,juce::dontSendNotification);mixerPan_.setValue(0.0,juce::dontSendNotification);muteTrack_.setButtonText("Mute");soloTrack_.setButtonText("Solo");trackMeterLabel_.setText("MASTER METER",juce::dontSendNotification);}
        else if(channel){mixerVolume_.setValue(channel->volume,juce::dontSendNotification);mixerPan_.setValue(channel->pan,juce::dontSendNotification);muteTrack_.setButtonText(channel->mute?"Muted":"Mute");soloTrack_.setButtonText(channel->solo?"Soloed":"Solo");trackMeterLabel_.setText(mixerTargetChoice_.getSelectedId()>=100?"BUS METER":"TRACK METER",juce::dontSendNotification);}
        else{mixerVolume_.setValue(1.0,juce::dontSendNotification);mixerPan_.setValue(0.0,juce::dontSendNotification);muteTrack_.setButtonText("Mute");soloTrack_.setButtonText("Solo");trackMeterLabel_.setText("CHANNEL METER",juce::dontSendNotification);trackMeter_.clear();}
        suppressMixerCallbacks_=false;
    }
    void beginMixerGesture(){if(mixerGestureActive_)return;mixerBefore_=project_;mixerGestureActive_=true;}
    void applyMixerSliders(){
        if(suppressMixerCallbacks_)return;const bool master=mixerTargetChoice_.getSelectedId()==2;auto*channel=selectedMixerChannel();if(!master&&!channel)return;if(!mixerGestureActive_)beginMixerGesture();
        if(master)project_.master.volume=static_cast<float>(mixerVolume_.getValue());else{channel->volume=static_cast<float>(mixerVolume_.getValue());channel->pan=static_cast<float>(mixerPan_.getValue());}
        engine_.publish(project_);if(arrangement_)arrangement_->repaint();
    }
    void endMixerGesture(){if(!mixerGestureActive_)return;mixerGestureActive_=false;undo_.commit(std::move(mixerBefore_),project_,"Edit "+activeMixerName().toStdString()+" mixer channel");status_.setText(activeMixerName()+" mixer edit committed",juce::dontSendNotification);}
    void toggleMixerFlag(bool solo){
        auto*channel=selectedMixerChannel();if(!channel)return;Project before=project_;if(solo)channel->solo=!channel->solo;else channel->mute=!channel->mute;undo_.commit(std::move(before),project_,solo?"Toggle mixer solo":"Toggle mixer mute");publishEdit(activeMixerName()+(solo?" solo changed":" mute changed"));
    }
    Track* selectedMixerTrack(){
        if(mixerTargetChoice_.getSelectedId()!=1)return nullptr;const int index=selectedTrackIndex();return index>=0?&project_.tracks[static_cast<std::size_t>(index)]:nullptr;
    }
    int selectedMixerSendBusIndex()const{const int index=mixerSendBusChoice_.getSelectedId()-1;return index>=0&&index<static_cast<int>(project_.buses.size())?index:-1;}
    MixerSend* selectedMixerSend(){
        auto*track=selectedMixerTrack();const int busIndex=selectedMixerSendBusIndex();if(!track||busIndex<0)return nullptr;const Id busId=project_.buses[static_cast<std::size_t>(busIndex)].id;
        auto it=std::find_if(track->sends.begin(),track->sends.end(),[&](auto const&send){return send.busId==busId;});return it==track->sends.end()?nullptr:&*it;
    }
    void refreshMixerRoutingControls(){
        suppressMixerCallbacks_=true;auto*track=selectedMixerTrack();const bool enabled=track!=nullptr;
        const int previousOutput=mixerOutputChoice_.getSelectedId(),previousSend=mixerSendBusChoice_.getSelectedId();
        mixerOutputChoice_.clear(juce::dontSendNotification);mixerOutputChoice_.addItem("OUTPUT • Master",1);int outputId=100;for(auto const&bus:project_.buses)mixerOutputChoice_.addItem("OUTPUT • "+juce::String(bus.name),outputId++);
        mixerSendBusChoice_.clear(juce::dontSendNotification);int sendId=1;for(auto const&bus:project_.buses)mixerSendBusChoice_.addItem("SEND • "+juce::String(bus.name),sendId++);
        mixerOutputChoice_.setEnabled(enabled);mixerSendBusChoice_.setEnabled(enabled&&!project_.buses.empty());mixerSendGain_.setEnabled(enabled&&!project_.buses.empty());mixerSendPre_.setEnabled(enabled&&!project_.buses.empty());
        if(track){
            int selectedOutput=1;if(track->outputBusId!=0)for(std::size_t i=0;i<project_.buses.size();++i)if(project_.buses[i].id==track->outputBusId){selectedOutput=100+static_cast<int>(i);break;}
            mixerOutputChoice_.setSelectedId(selectedOutput,juce::dontSendNotification);
            if(!project_.buses.empty())mixerSendBusChoice_.setSelectedId(previousSend>0&&previousSend<=static_cast<int>(project_.buses.size())?previousSend:1,juce::dontSendNotification);
        }else mixerOutputChoice_.setSelectedId(previousOutput>0?previousOutput:1,juce::dontSendNotification);
        suppressMixerCallbacks_=false;syncMixerSendControls();
    }
    void syncMixerSendControls(){
        suppressMixerCallbacks_=true;auto*send=selectedMixerSend();const bool valid=selectedMixerTrack()!=nullptr&&selectedMixerSendBusIndex()>=0;
        mixerSendGain_.setValue(send?send->gain:0.0f,juce::dontSendNotification);mixerSendPre_.setToggleState(send?send->preFader:false,juce::dontSendNotification);mixerSetSend_.setEnabled(valid);mixerRemoveSend_.setEnabled(send!=nullptr);mixerSetSend_.setButtonText(send?"Update Send":"Set Send");suppressMixerCallbacks_=false;
    }
    void setMixerOutputRoute(){
        auto*track=selectedMixerTrack();if(!track)return;const int selected=mixerOutputChoice_.getSelectedId();Id busId=0;if(selected>=100){const int index=selected-100;if(index<0||index>=static_cast<int>(project_.buses.size()))return;busId=project_.buses[static_cast<std::size_t>(index)].id;}if(track->outputBusId==busId)return;
        Project before=project_;track->outputBusId=busId;undo_.commit(std::move(before),project_,"Change track output route");publishEdit("Track output routed to "+(busId==0?juce::String("Master"):juce::String(project_.findBus(busId)?project_.findBus(busId)->name:"Bus")));
    }
    void setMixerSend(){
        auto*track=selectedMixerTrack();const int busIndex=selectedMixerSendBusIndex();if(!track||busIndex<0)return;const Id busId=project_.buses[static_cast<std::size_t>(busIndex)].id;Project before=project_;
        auto it=std::find_if(track->sends.begin(),track->sends.end(),[&](auto const&send){return send.busId==busId;});if(it==track->sends.end()){MixerSend send;send.busId=busId;send.gain=static_cast<float>(mixerSendGain_.getValue());send.preFader=mixerSendPre_.getToggleState();track->sends.push_back(send);}else{it->gain=static_cast<float>(mixerSendGain_.getValue());it->preFader=mixerSendPre_.getToggleState();it->enabled=true;}
        undo_.commit(std::move(before),project_,"Set track send");publishEdit("Send to "+juce::String(project_.buses[static_cast<std::size_t>(busIndex)].name)+" updated");
    }
    void removeMixerSend(){
        auto*track=selectedMixerTrack();const int busIndex=selectedMixerSendBusIndex();if(!track||busIndex<0)return;const Id busId=project_.buses[static_cast<std::size_t>(busIndex)].id;auto it=std::find_if(track->sends.begin(),track->sends.end(),[&](auto const&send){return send.busId==busId;});if(it==track->sends.end())return;
        Project before=project_;track->sends.erase(it);undo_.commit(std::move(before),project_,"Remove track send");publishEdit("Send removed");
    }
    void assignSelectedInstrument(){
        const int trackIndex=selectedTrackIndex(),pluginIndex=selectedPluginIndex();
        if(trackIndex<0){status_.setText("Select a track first",juce::dontSendNotification);return;}
        if(pluginIndex<0||pluginIndex>=static_cast<int>(plugins_.size())){status_.setText("Scan and select an instrument first",juce::dontSendNotification);return;}
        const auto&descriptor=plugins_[static_cast<std::size_t>(pluginIndex)];
        if(!descriptor.instrument){status_.setText("Selected plugin is an effect, not an instrument",juce::dontSendNotification);return;}
        Project before=project_;auto&track=project_.tracks[static_cast<std::size_t>(trackIndex)];PluginInstance instance;instance.format=descriptor.format;instance.identifier=descriptor.identifier;instance.name=descriptor.name;instance.enabled=true;instance.bypass=false;instance.wet=1.0f;track.externalInstrument=std::move(instance);track.externalInstrumentEnabled=true;undo_.commit(std::move(before),project_,"Set external instrument");publishEdit("Instrument assigned to "+juce::String(track.name));
    }
    void clearSelectedInstrument(){
        const int trackIndex=selectedTrackIndex();if(trackIndex<0){status_.setText("Select a track first",juce::dontSendNotification);return;}
        auto&track=project_.tracks[static_cast<std::size_t>(trackIndex)];if(!track.externalInstrumentEnabled){status_.setText("Selected track has no external instrument",juce::dontSendNotification);return;}
        Project before=project_;const auto name=track.externalInstrument.name;track.externalInstrumentEnabled=false;track.externalInstrument=PluginInstance{};undo_.commit(std::move(before),project_,"Clear external instrument");publishEdit("Cleared instrument "+juce::String(name));
    }
    void addSelectedEffect(bool master){
        const int pluginIndex=selectedPluginIndex();if(pluginIndex<0||pluginIndex>=static_cast<int>(plugins_.size())){status_.setText("Scan and select an effect first",juce::dontSendNotification);return;}
        const auto&descriptor=plugins_[static_cast<std::size_t>(pluginIndex)];if(descriptor.instrument){status_.setText("Selected plugin is an instrument, not an effect insert",juce::dontSendNotification);return;}
        if(!master&&selectedTrackIndex()<0){status_.setText("Select a track first",juce::dontSendNotification);return;}
        PluginInstance instance;instance.format=descriptor.format;instance.identifier=descriptor.identifier;instance.name=descriptor.name;instance.enabled=true;instance.bypass=false;instance.wet=1.0f;
        Project before=project_;if(master)project_.master.plugins.push_back(instance);else project_.tracks[static_cast<std::size_t>(selectedTrackIndex())].mixer.plugins.push_back(instance);
        undo_.commit(std::move(before),project_,master?"Add master insert":"Add track insert");publishEdit((master?"Master FX: ":"Track FX: ")+juce::String(descriptor.name));
    }
    void undoEdit(){if(!undo_.undo(project_)){status_.setText("Nothing to undo",juce::dontSendNotification);return;}publishEdit("Undo");}
    void redoEdit(){if(!undo_.redo(project_)){status_.setText("Nothing to redo",juce::dontSendNotification);return;}publishEdit("Redo");}
    void writeProject(std::filesystem::path path){
        try{if(path.extension()!=".flow")path+=".flow";ProjectSerializer::save(project_,path);projectPath_=path;settings_.lastProjectPath=path;recoveredAtStartup_=false;preserveRecoveryOnExit_=false;resetRecoverySnapshot();projectLabel_.setText("Project: "+juce::String(path.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);status_.setText("Project saved (v11)",juce::dontSendNotification);}
        catch(const std::exception&e){status_.setText("Save failed: "+juce::String(e.what()),juce::dontSendNotification);}
    }
    void saveProject(){
        if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();
        if(!projectPath_.empty()){writeProject(projectPath_);return;}
        chooser_=std::make_unique<juce::FileChooser>("Save FLOWDAW project",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("FLOWDAW.flow"),"*.flow");
        chooser_->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser&fc){auto file=fc.getResult();if(file!=juce::File{})writeProject(std::filesystem::path(file.getFullPathName().toStdString()));chooser_.reset();});
    }
    juce::AudioPluginFormat* formatFor(const PluginDescriptor&p){for(auto*f:formatManager_.getFormats()){auto n=f->getName().toLowerCase();if((p.format=="vst3"&&n.contains("vst3"))||(p.format=="au"&&n.contains("audio")))return f;}return nullptr;}
    void openSelectedEditor(){const int idx=selectedPluginIndex();if(idx<0||idx>=static_cast<int>(plugins_.size())){status_.setText("Scan and select a plugin first",juce::dontSendNotification);return;}auto p=plugins_[static_cast<std::size_t>(idx)];if(safety_.isQuarantined(p.identifier)){status_.setText("Plugin is quarantined; clear it with FLOWDAW Doctor before retrying",juce::dontSendNotification);return;}auto*fmt=formatFor(p);if(!fmt){status_.setText("No JUCE format backend for selection",juce::dontSendNotification);return;}juce::OwnedArray<juce::PluginDescription> desc;fmt->findAllTypesForFile(desc,juce::String(p.identifier));if(desc.isEmpty()){safety_.noteFailure(p.identifier,"Plugin description could not be recreated");saveSafety();status_.setText("Plugin description could not be recreated",juce::dontSendNotification);return;}auto d=*desc[0];auto setup=deviceManager_.getAudioDeviceSetup();formatManager_.createPluginInstanceAsync(d,setup.sampleRate>0?setup.sampleRate:48000.0,setup.bufferSize>0?setup.bufferSize:256,[this,id=p.identifier](std::unique_ptr<juce::AudioPluginInstance>instance,const juce::String&error){if(!instance){safety_.noteFailure(id,error.toStdString());saveSafety();status_.setText("Plugin load failed: "+error,juce::dontSendNotification);return;}safety_.noteSuccess(id);saveSafety();pluginWindow_=std::make_unique<PluginEditorWindow>(std::move(instance));status_.setText("Plugin editor hosted in JUCE window",juce::dontSendNotification);});}
    void timerCallback()override{
        engine_.collectRetiredGraphs();
        syncTransportVisualState();
        recChops_.setButtonText(recordingChops_?"STOP CHOPS":"REC CHOPS");recAudio_.setButtonText(audioRecording_?"STOP Audio":"REC Audio");
        if(arrangement_){const auto tick=MusicalTime::samplesToTicks(engine_.playheadSamples(),project_.transport.bpm,engine_.sampleRate());arrangement_->setPlayheadTick(tick);}
        const auto meters=engine_.meterSnapshot();
        const auto db=[](float value){return value>0.000001f?20.0f*std::log10(value):-120.0f;};
        meterLabel_.setText("MASTER • TP "
                            +juce::String(std::max(db(meters.master.truePeakLeft),db(meters.master.truePeakRight)),1)
                            +" dB • PK "+juce::String(std::max(db(meters.master.samplePeakLeft),db(meters.master.samplePeakRight)),1)
                            +" dB • RMS "+juce::String(std::max(db(meters.master.rmsLeft),db(meters.master.rmsRight)),1)+" dB",juce::dontSendNotification);
        const int mixerTarget=mixerTargetChoice_.getSelectedId();bool found=false;
        if(mixerTarget==2){trackMeter_.setReading(meters.master);found=true;}
        else if(mixerTarget>=100){const int busIndex=selectedMixerBusIndex();if(busIndex>=0){const Id id=project_.buses[static_cast<std::size_t>(busIndex)].id;for(auto const&route:meters.buses)if(route.id==id){trackMeter_.setReading(route.level);found=true;break;}}}
        else{const int selected=selectedTrackIndex();if(selected>=0){const Id id=project_.tracks[static_cast<std::size_t>(selected)].id;for(auto const&route:meters.tracks)if(route.id==id){trackMeter_.setReading(route.level);found=true;break;}}}
        if(!found)trackMeter_.clear();
        if(++settingsSaveTicks_>=50){settingsSaveTicks_=0;saveDeviceSettings();}
        if(++autosaveTicks_>=std::max(1,settings_.autosaveSeconds*10)){autosaveTicks_=0;autosaveRecovery();}
    }
    void autosaveRecovery(){if(!recovery_)return;try{recovery_->autosave(project_,projectPath_);}catch(const std::exception&e){status_.setText("Autosave failed: "+juce::String(e.what()),juce::dontSendNotification);}}
    void resetRecoverySnapshot(){autosaveTicks_=0;autosaveRecovery();}
    void saveSafety(){try{safety_.save(safetyPath_);}catch(...){} }
    void saveDeviceSettings(){auto s=deviceManager_.getAudioDeviceSetup();settings_.audio.preferredSampleRate=s.sampleRate>0?static_cast<int>(s.sampleRate):48000;settings_.audio.bufferSize=sanitizeBufferSize(static_cast<unsigned long>(std::max(1,s.bufferSize)));settings_.audio.inputDevice=s.inputDeviceName.toStdString();settings_.audio.outputDevice=s.outputDeviceName.toStdString();try{saveAppSettings(settings_,settingsPath_);}catch(...){} }
    AppSettings settings_;PluginSafetyRegistry safety_;std::unique_ptr<SessionRecovery> recovery_;std::filesystem::path settingsPath_,safetyPath_,projectPath_;Project project_;UndoStack undo_;AudioEngine engine_;std::shared_ptr<PluginHost>pluginHost_;juce::AudioDeviceManager deviceManager_;juce::AudioPluginFormatManager formatManager_;std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;std::unique_ptr<juce::FileChooser> chooser_;std::vector<PluginDescriptor> plugins_;std::vector<int> visiblePluginIndices_;std::unique_ptr<PluginEditorWindow> pluginWindow_;std::unique_ptr<juceui::ArrangementComponent> arrangement_;std::unique_ptr<juceui::PianoRollComponent> piano_;std::unique_ptr<juceui::SamplerComponent> sampler_;std::unique_ptr<juceui::StepSequencerComponent> sequencer_;std::unique_ptr<juceui::AutomationAssistComponent> automationAssist_;std::unique_ptr<juceui::SampleBrowserComponent> sampleBrowser_;EditorMode editorMode_=EditorMode::Arrangement;std::array<float,kMaxDeviceBlock>monoInput_{};std::array<float,kMaxDeviceBlock*2>stereoOutput_{};juceui::ShellLookAndFeel shellLookAndFeel_;juce::Label title_,status_,projectLabel_,note_,meterLabel_,bpmLabel_,mixerSectionLabel_,mixerRoutingLabel_,mixerVolumeLabel_,mixerPanLabel_,trackMeterLabel_,rackParamLabel_;int settingsSaveTicks_=0,autosaveTicks_=0;bool recoveredAtStartup_=false,preserveRecoveryOnExit_=false,suppressMixerCallbacks_=false,mixerGestureActive_=false,suppressRackCallbacks_=false,rackGestureActive_=false,rackEditorDirty_=false,suppressChopCallbacks_=false,chopGestureActive_=false,recordingChops_=false,audioRecording_=false;Id chopRecordPatternId_=0,audioRecordTrackId_=0,rackEditorPluginId_=0;Tick chopRecordStartTick_=0,audioRecordStartTick_=0;Project mixerBefore_,rackBefore_,rackEditorBefore_,chopBefore_,chopRecordBefore_,audioRecordBefore_;juce::TextButton newProject_,loadProject_,importWav_,saveProject_,play_,stop_,bpmMinus_,bpmPlus_,undoButton_,redoButton_,commandPalette_,scan_,openEditor_,setInstrument_,clearInstrument_,addTrackFx_,addMasterFx_,muteTrack_,soloTrack_,mixerSetSend_,mixerRemoveSend_,arrangementTab_,pianoTab_,sequencerTab_,automationTab_,samplerTab_,bankPrev_,bankNext_,analyzeSample_,chop8_,autoChop_,chopBeat_,chopBar_,matchBpm_,exportMix_,exportStems_,stopPreview_,recChops_,chopReset_,recAudio_,monitorInput_,prevTake_,nextTake_,renamePad_,padGainMinus_,padGainPlus_,padPanMinus_,padPanPlus_,padChokeMinus_,padChokePlus_,addRackGain_,addRackClip_,addRackWidth_,addRackExternal_,rackMoveUp_,rackMoveDown_,rackEnabled_,rackBypass_,rackRemove_,openRackEditor_;juce::ComboBox pluginChoice_,pluginKindChoice_,trackChoice_,mixerTargetChoice_,mixerOutputChoice_,mixerSendBusChoice_,patternChoice_,sampleChoice_,rackTargetChoice_,rackPluginChoice_,chopGridChoice_;juce::TextEditor pluginSearch_,padName_;juce::Slider mixerVolume_,mixerPan_,mixerSendGain_,rackWet_,rackParam_,chopQuantizeStrength_,chopHumanizeStrength_;juce::ToggleButton mixerSendPre_;juceui::StereoMeterComponent trackMeter_;
};

class MainWindow final:public juce::DocumentWindow{
public:MainWindow():DocumentWindow("FLOWDAW",juce::Colours::black,DocumentWindow::allButtons){setUsingNativeTitleBar(true);setContentOwned(new MainComponent(),true);centreWithSize(getWidth(),getHeight());setVisible(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
};
class FlowdawApplication final:public juce::JUCEApplication{
public:const juce::String getApplicationName()override{return"FLOWDAW";}const juce::String getApplicationVersion()override{return"0.8.0";}bool moreThanOneInstanceAllowed()override{return true;}void initialise(const juce::String&)override{window_=std::make_unique<MainWindow>();}void shutdown()override{window_.reset();}void systemRequestedQuit()override{quit();}void anotherInstanceStarted(const juce::String&)override{}
private:std::unique_ptr<MainWindow>window_;
};
}
START_JUCE_APPLICATION(FlowdawApplication)