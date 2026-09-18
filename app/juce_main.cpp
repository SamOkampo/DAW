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
#include "flowdaw/Undo.hpp"
#include "JuceEditingSurface.hpp"
#include "JucePianoSamplerSurface.hpp"
#include "JuceStepSequencerSurface.hpp"
#include "JuceAutomationAssistSurface.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <cmath>
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

class MainComponent final:public juce::Component,private juce::Timer,private juce::AudioIODeviceCallback{
public:
    MainComponent(){
        pluginHost_=std::make_shared<PluginHost>();pluginHost_->registerBackend(makeJucePluginBackend());engine_.setPluginHost(pluginHost_);
        const auto config=defaultSettingsDirectory();settingsPath_=config/"settings.conf";safetyPath_=config/"plugin-safety.conf";settings_=std::filesystem::exists(settingsPath_)?loadAppSettings(settingsPath_):defaultAppSettings();safety_.load(safetyPath_);
        bool loadedLast=false;if(!settings_.lastProjectPath.empty()&&std::filesystem::exists(settings_.lastProjectPath))try{project_=ProjectSerializer::load(settings_.lastProjectPath,true);projectPath_=settings_.lastProjectPath;loadedLast=true;}catch(...){}if(!loadedLast)project_=makeStarterProject();
        juce::String err=deviceManager_.initialise(2,2,nullptr,true);auto setup=deviceManager_.getAudioDeviceSetup();setup.sampleRate=settings_.audio.preferredSampleRate;setup.bufferSize=static_cast<int>(sanitizeBufferSize(settings_.audio.bufferSize));
        if(!settings_.audio.inputDevice.empty())setup.inputDeviceName=settings_.audio.inputDevice;if(!settings_.audio.outputDevice.empty())setup.outputDeviceName=settings_.audio.outputDevice;deviceManager_.setAudioDeviceSetup(setup,true);
        selector_=std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager_,0,2,0,2,true,true,true,false);addAndMakeVisible(*selector_);deviceManager_.addAudioCallback(this);
        addHostFormats(formatManager_);
        title_.setText("FLOWDAW JUCE Runtime",juce::dontSendNotification);title_.setFont(juce::Font(24.0f,juce::Font::bold));addAndMakeVisible(title_);
        status_.setText(err.isEmpty()?"JUCE AudioDeviceManager drives FLOWDAW core":"Audio: "+err,juce::dontSendNotification);addAndMakeVisible(status_);
        projectLabel_.setText(projectPath_.empty()?"Untitled Beat • starter project":"Project: "+juce::String(projectPath_.filename().string()),juce::dontSendNotification);addAndMakeVisible(projectLabel_);
        newProject_.setButtonText("New");newProject_.onClick=[this]{newProject();};addAndMakeVisible(newProject_);
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
        play_.setButtonText("Play");play_.onClick=[this]{if(engine_.isPlaying()){engine_.pause();status_.setText("Paused",juce::dontSendNotification);}else{engine_.play();status_.setText("Playing through JUCE device",juce::dontSendNotification);}};addAndMakeVisible(play_);
        bpmMinus_.setButtonText("- BPM");bpmMinus_.onClick=[this]{changeBpm(-1.0);};addAndMakeVisible(bpmMinus_);
        bpmPlus_.setButtonText("+ BPM");bpmPlus_.onClick=[this]{changeBpm(1.0);};addAndMakeVisible(bpmPlus_);
        bpmLabel_.setJustificationType(juce::Justification::centred);addAndMakeVisible(bpmLabel_);
        stop_.setButtonText("Stop");stop_.onClick=[this]{engine_.stop();status_.setText("Stopped",juce::dontSendNotification);};addAndMakeVisible(stop_);
        scan_.setButtonText("Scan VST3/AU");scan_.onClick=[this]{scanPlugins();};addAndMakeVisible(scan_);
        openEditor_.setButtonText("Preview Scanned Plugin");openEditor_.onClick=[this]{openSelectedEditor();};addAndMakeVisible(openEditor_);
        addAndMakeVisible(pluginChoice_);pluginChoice_.setTextWhenNothingSelected("No plugin selected");
        refreshRackTargets();rackTargetChoice_.setSelectedId(1,juce::dontSendNotification);rackTargetChoice_.onChange=[this]{if(!suppressRackCallbacks_)refreshRackControls();};addAndMakeVisible(rackTargetChoice_);
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
        trackChoice_.onChange=[this]{syncMixerControls();refreshRackControls();};
        mixerVolume_.setRange(0.0,2.0,0.01);mixerVolume_.setSliderStyle(juce::Slider::LinearHorizontal);mixerVolume_.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);mixerVolume_.onDragStart=[this]{beginMixerGesture();};mixerVolume_.onValueChange=[this]{applyMixerSliders();};mixerVolume_.onDragEnd=[this]{endMixerGesture();};addAndMakeVisible(mixerVolume_);
        mixerPan_.setRange(-1.0,1.0,0.01);mixerPan_.setSliderStyle(juce::Slider::LinearHorizontal);mixerPan_.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);mixerPan_.onDragStart=[this]{beginMixerGesture();};mixerPan_.onValueChange=[this]{applyMixerSliders();};mixerPan_.onDragEnd=[this]{endMixerGesture();};addAndMakeVisible(mixerPan_);
        mixerVolumeLabel_.setText("Track Volume",juce::dontSendNotification);mixerPanLabel_.setText("Pan",juce::dontSendNotification);addAndMakeVisible(mixerVolumeLabel_);addAndMakeVisible(mixerPanLabel_);
        muteTrack_.setButtonText("Mute");muteTrack_.onClick=[this]{toggleMixerFlag(false);};addAndMakeVisible(muteTrack_);
        soloTrack_.setButtonText("Solo");soloTrack_.onClick=[this]{toggleMixerFlag(true);};addAndMakeVisible(soloTrack_);
        trackMeterLabel_.setText("Selected track meter",juce::dontSendNotification);addAndMakeVisible(trackMeterLabel_);addAndMakeVisible(trackMeter_);
        auto commitEdit=[this](Project before,std::string name){undo_.commit(std::move(before),project_,name);publishEdit(juce::String(name));};
        arrangement_=std::make_unique<juceui::ArrangementComponent>(project_,commitEdit);addAndMakeVisible(*arrangement_);
        piano_=std::make_unique<juceui::PianoRollComponent>(project_,engine_,commitEdit);addChildComponent(*piano_);
        sampler_=std::make_unique<juceui::SamplerComponent>(project_,engine_,commitEdit,[this](Id sampleId,Id sliceId){recordChopTrigger(sampleId,sliceId);});addChildComponent(*sampler_);
        sequencer_=std::make_unique<juceui::StepSequencerComponent>(project_,commitEdit);addChildComponent(*sequencer_);
        automationAssist_=std::make_unique<juceui::AutomationAssistComponent>(project_,[this]{return MusicalTime::samplesToTicks(engine_.playheadSamples(),project_.transport.bpm,engine_.sampleRate());},commitEdit);addChildComponent(*automationAssist_);
        arrangementTab_.setButtonText("Arrangement");arrangementTab_.onClick=[this]{setEditorMode(EditorMode::Arrangement);};addAndMakeVisible(arrangementTab_);
        pianoTab_.setButtonText("Piano Roll");pianoTab_.onClick=[this]{setEditorMode(EditorMode::Piano);};addAndMakeVisible(pianoTab_);
        sequencerTab_.setButtonText("Sequencer");sequencerTab_.onClick=[this]{setEditorMode(EditorMode::Step);};addAndMakeVisible(sequencerTab_);
        automationTab_.setButtonText("Automation");automationTab_.onClick=[this]{setEditorMode(EditorMode::Automation);};addAndMakeVisible(automationTab_);
        samplerTab_.setButtonText("Sampler");samplerTab_.onClick=[this]{setEditorMode(EditorMode::Sampler);};addAndMakeVisible(samplerTab_);
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
        refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshRackControls();syncChopControls();updateBpmLabel();setEditorMode(EditorMode::Arrangement);
        note_.setText("JUCE owns device I/O and registers the real VST3/AU backend with FLOWDAW's AudioEngine. Phase 8 now executes prepared track, bus and master inserts through preallocated realtime route buffers with PDC; full Studio editing parity remains in progress.",juce::dontSendNotification);note_.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(note_);
        meterLabel_.setText("Meters (TP estimate / sample peak / RMS): waiting for audio",juce::dontSendNotification);addAndMakeVisible(meterLabel_);
        setSize(1440,1040);startTimer(100);
    }
    ~MainComponent()override{if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();pluginWindow_.reset();deviceManager_.removeAudioCallback(this);engine_.stop();engine_.collectRetiredGraphs();saveDeviceSettings();saveSafety();}
    void resized()override{
        auto r=getLocalBounds().reduced(16);title_.setBounds(r.removeFromTop(38));status_.setBounds(r.removeFromTop(26));projectLabel_.setBounds(r.removeFromTop(26));note_.setBounds(r.removeFromTop(54));meterLabel_.setBounds(r.removeFromTop(28));
        auto transport=r.removeFromTop(38);newProject_.setBounds(transport.removeFromLeft(72).reduced(3));loadProject_.setBounds(transport.removeFromLeft(100).reduced(3));importWav_.setBounds(transport.removeFromLeft(100).reduced(3));saveProject_.setBounds(transport.removeFromLeft(105).reduced(3));play_.setBounds(transport.removeFromLeft(66).reduced(3));stop_.setBounds(transport.removeFromLeft(66).reduced(3));bpmMinus_.setBounds(transport.removeFromLeft(60).reduced(3));bpmLabel_.setBounds(transport.removeFromLeft(80).reduced(3));bpmPlus_.setBounds(transport.removeFromLeft(60).reduced(3));undoButton_.setBounds(transport.removeFromLeft(66).reduced(3));redoButton_.setBounds(transport.removeFromLeft(66).reduced(3));
        auto controls=r.removeFromTop(38);scan_.setBounds(controls.removeFromLeft(150).reduced(3));pluginChoice_.setBounds(controls.removeFromLeft(430).reduced(3));openEditor_.setBounds(controls.removeFromLeft(180).reduced(3));
        auto instrument=r.removeFromTop(38);trackChoice_.setBounds(instrument.removeFromLeft(300).reduced(3));setInstrument_.setBounds(instrument.removeFromLeft(130).reduced(3));clearInstrument_.setBounds(instrument.removeFromLeft(135).reduced(3));addTrackFx_.setBounds(instrument.removeFromLeft(120).reduced(3));addMasterFx_.setBounds(instrument.removeFromLeft(125).reduced(3));
        auto rack=r.removeFromTop(38);rackTargetChoice_.setBounds(rack.removeFromLeft(150).reduced(3));rackPluginChoice_.setBounds(rack.removeFromLeft(205).reduced(3));addRackGain_.setBounds(rack.removeFromLeft(92).reduced(3));addRackClip_.setBounds(rack.removeFromLeft(82).reduced(3));addRackWidth_.setBounds(rack.removeFromLeft(70).reduced(3));addRackExternal_.setBounds(rack.removeFromLeft(78).reduced(3));rackMoveUp_.setBounds(rack.removeFromLeft(38).reduced(3));rackMoveDown_.setBounds(rack.removeFromLeft(45).reduced(3));rackEnabled_.setBounds(rack.removeFromLeft(68).reduced(3));rackBypass_.setBounds(rack.removeFromLeft(62).reduced(3));rackRemove_.setBounds(rack.removeFromLeft(62).reduced(3));openRackEditor_.setBounds(rack.removeFromLeft(90).reduced(3));rackWet_.setBounds(rack.removeFromLeft(120).reduced(3));rackParamLabel_.setBounds(rack.removeFromLeft(60).reduced(3));rackParam_.setBounds(rack.reduced(3));
        auto editorBar=r.removeFromTop(38);arrangementTab_.setBounds(editorBar.removeFromLeft(105).reduced(3));pianoTab_.setBounds(editorBar.removeFromLeft(95).reduced(3));sequencerTab_.setBounds(editorBar.removeFromLeft(95).reduced(3));automationTab_.setBounds(editorBar.removeFromLeft(100).reduced(3));samplerTab_.setBounds(editorBar.removeFromLeft(85).reduced(3));patternChoice_.setBounds(editorBar.removeFromLeft(245).reduced(3));sampleChoice_.setBounds(editorBar.removeFromLeft(255).reduced(3));bankPrev_.setBounds(editorBar.removeFromLeft(72).reduced(3));bankNext_.setBounds(editorBar.removeFromLeft(72).reduced(3));
        auto sampleTools=r.removeFromTop(38);analyzeSample_.setBounds(sampleTools.removeFromLeft(82).reduced(3));chop8_.setBounds(sampleTools.removeFromLeft(72).reduced(3));autoChop_.setBounds(sampleTools.removeFromLeft(88).reduced(3));chopBeat_.setBounds(sampleTools.removeFromLeft(88).reduced(3));chopBar_.setBounds(sampleTools.removeFromLeft(84).reduced(3));matchBpm_.setBounds(sampleTools.removeFromLeft(92).reduced(3));exportMix_.setBounds(sampleTools.removeFromLeft(90).reduced(3));exportStems_.setBounds(sampleTools.removeFromLeft(100).reduced(3));padName_.setBounds(sampleTools.removeFromLeft(118).reduced(3));renamePad_.setBounds(sampleTools.removeFromLeft(70).reduced(3));padGainMinus_.setBounds(sampleTools.removeFromLeft(64).reduced(3));padGainPlus_.setBounds(sampleTools.removeFromLeft(64).reduced(3));padPanMinus_.setBounds(sampleTools.removeFromLeft(60).reduced(3));padPanPlus_.setBounds(sampleTools.removeFromLeft(60).reduced(3));
        auto recordTools=r.removeFromTop(38);stopPreview_.setBounds(recordTools.removeFromLeft(88).reduced(3));recChops_.setBounds(recordTools.removeFromLeft(88).reduced(3));chopGridChoice_.setBounds(recordTools.removeFromLeft(92).reduced(3));chopQuantizeStrength_.setBounds(recordTools.removeFromLeft(160).reduced(3));chopHumanizeStrength_.setBounds(recordTools.removeFromLeft(160).reduced(3));chopReset_.setBounds(recordTools.removeFromLeft(82).reduced(3));recAudio_.setBounds(recordTools.removeFromLeft(88).reduced(3));monitorInput_.setBounds(recordTools.removeFromLeft(80).reduced(3));prevTake_.setBounds(recordTools.removeFromLeft(70).reduced(3));nextTake_.setBounds(recordTools.removeFromLeft(70).reduced(3));padChokeMinus_.setBounds(recordTools.removeFromLeft(74).reduced(3));padChokePlus_.setBounds(recordTools.removeFromLeft(74).reduced(3));
        r.removeFromTop(4);auto editorArea=r.removeFromTop(320);if(arrangement_)arrangement_->setBounds(editorArea);if(piano_)piano_->setBounds(editorArea);if(sampler_)sampler_->setBounds(editorArea);if(sequencer_)sequencer_->setBounds(editorArea);if(automationAssist_)automationAssist_->setBounds(editorArea);
        auto mixer=r.removeFromTop(118);mixerVolumeLabel_.setBounds(mixer.removeFromLeft(95).reduced(3));mixerVolume_.setBounds(mixer.removeFromLeft(245).reduced(3));mixerPanLabel_.setBounds(mixer.removeFromLeft(45).reduced(3));mixerPan_.setBounds(mixer.removeFromLeft(220).reduced(3));muteTrack_.setBounds(mixer.removeFromLeft(70).reduced(3));soloTrack_.setBounds(mixer.removeFromLeft(70).reduced(3));trackMeterLabel_.setBounds(mixer.removeFromLeft(120).reduced(3));trackMeter_.setBounds(mixer.reduced(3));
        r.removeFromTop(6);selector_->setBounds(r);
    }
private:
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
    void newProject(){
        if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();pluginWindow_.reset();engine_.stop();project_=makeStarterProject();projectPath_.clear();settings_.lastProjectPath.clear();undo_=UndoStack{};engine_.publish(project_);
        refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshRackControls();syncChopControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();
        projectLabel_.setText("Untitled Beat • starter project",juce::dontSendNotification);status_.setText("New project: drums + FLOW Keys ready",juce::dontSendNotification);saveDeviceSettings();
    }
    void refreshRackTargets(){
        const int previous=rackTargetChoice_.getSelectedId();rackTargetChoice_.clear(juce::dontSendNotification);rackTargetChoice_.addItem("Master Rack",1);rackTargetChoice_.addItem("Selected Track Rack",2);
        int id=100;for(auto const&bus:project_.buses)rackTargetChoice_.addItem("Bus: "+juce::String(bus.name),id++);
        const bool previousValid=previous==1||previous==2||(previous>=100&&previous<100+static_cast<int>(project_.buses.size()));rackTargetChoice_.setSelectedId(previousValid?previous:1,juce::dontSendNotification);
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
        suppressRackCallbacks_=true;refreshRackTargets();const int previous=rackPluginChoice_.getSelectedId();rackPluginChoice_.clear(juce::dontSendNotification);auto*rack=rackPlugins();if(rack){int id=1;for(auto const&p:*rack){auto label=juce::String(p.name)+" ["+juce::String(p.format)+"] "+(!p.enabled?"DISABLED":(p.bypass?"BYPASS":"ACTIVE"));rackPluginChoice_.addItem(label,id++);}if(!rack->empty())rackPluginChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(rack->size())?previous:1,juce::dontSendNotification);}suppressRackCallbacks_=false;syncRackControls();
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
        const int pluginIndex=pluginChoice_.getSelectedId()-1;if(pluginIndex<0||pluginIndex>=static_cast<int>(plugins_.size())){status_.setText("Scan and select an effect first",juce::dontSendNotification);return;}const auto&descriptor=plugins_[static_cast<std::size_t>(pluginIndex)];if(descriptor.instrument){status_.setText("Selected plugin is an instrument, not an effect insert",juce::dontSendNotification);return;}auto*rack=rackPlugins();if(!rack){status_.setText("Select a valid Track or Bus rack",juce::dontSendNotification);return;}
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
    void loadProjectFile(const std::filesystem::path&p){try{if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();pluginWindow_.reset();engine_.stop();project_=ProjectSerializer::load(p,true);projectPath_=p;settings_.lastProjectPath=p;undo_=UndoStack{};engine_.publish(project_);refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshRackControls();syncChopControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();projectLabel_.setText("Project: "+juce::String(p.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);status_.setText("Project loaded; Play uses JUCE device callback",juce::dontSendNotification);}catch(const std::exception&e){status_.setText("Open failed: "+juce::String(e.what()),juce::dontSendNotification);}}
    std::vector<std::filesystem::path> pluginRoots()const{
        if(!settings_.pluginRoots.empty())return settings_.pluginRoots;std::vector<std::filesystem::path> roots;auto home=std::filesystem::path(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName().toStdString());roots.push_back(home/".vst3");roots.push_back("/usr/lib/vst3");roots.push_back("/usr/local/lib/vst3");
#if JUCE_MAC
        roots.push_back(home/"Library/Audio/Plug-Ins/VST3");roots.push_back(home/"Library/Audio/Plug-Ins/Components");roots.push_back("/Library/Audio/Plug-Ins/VST3");roots.push_back("/Library/Audio/Plug-Ins/Components");
#endif
        return roots;
    }
    void scanPlugins(){std::string error;plugins_=scanPluginsWithJuce(pluginRoots(),error,&safety_);saveSafety();pluginChoice_.clear(juce::dontSendNotification);int id=1;for(auto const&p:plugins_){auto label=p.name+" ["+p.format+(p.instrument?"/instrument":"/effect")+"]";pluginChoice_.addItem(juce::String(label),id++);}if(!plugins_.empty())pluginChoice_.setSelectedId(1,juce::dontSendNotification);int quarantined=0;for(auto const&r:safety_.records())if(r.quarantined)++quarantined;auto base="JUCE scan: "+juce::String(static_cast<int>(plugins_.size()))+" plugin type(s), "+juce::String(quarantined)+" quarantined";status_.setText(error.empty()?base:base+" | "+juce::String(error),juce::dontSendNotification);}
    int selectedTrackIndex()const{const int index=trackChoice_.getSelectedId()-1;return index>=0&&index<static_cast<int>(project_.tracks.size())?index:-1;}
    void refreshTrackChoice(){
        const int previous=trackChoice_.getSelectedId();trackChoice_.clear(juce::dontSendNotification);int id=1;
        for(auto const&t:project_.tracks){std::string label=t.name;if(t.externalInstrumentEnabled&&!t.externalInstrument.name.empty())label+="  •  "+t.externalInstrument.name;trackChoice_.addItem(juce::String(label),id++);}
        if(!project_.tracks.empty())trackChoice_.setSelectedId(previous>0&&previous<=static_cast<int>(project_.tracks.size())?previous:1,juce::dontSendNotification);
    }
    void publishEdit(const juce::String&message){engine_.publish(project_);refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();refreshRackControls();syncChopControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();status_.setText(message,juce::dontSendNotification);}
    enum class EditorMode{Arrangement,Piano,Step,Automation,Sampler};
    void setEditorMode(EditorMode mode){
        editorMode_=mode;const bool arrangement=mode==EditorMode::Arrangement,piano=mode==EditorMode::Piano,step=mode==EditorMode::Step,automation=mode==EditorMode::Automation,sampler=mode==EditorMode::Sampler;
        if(arrangement_)arrangement_->setVisible(arrangement);if(piano_)piano_->setVisible(piano);if(sequencer_)sequencer_->setVisible(step);if(automationAssist_)automationAssist_->setVisible(automation);if(sampler_)sampler_->setVisible(sampler);
        patternChoice_.setVisible(piano||step);sampleChoice_.setVisible(sampler);bankPrev_.setVisible(sampler);bankNext_.setVisible(sampler);
        arrangementTab_.setEnabled(!arrangement);pianoTab_.setEnabled(!piano);sequencerTab_.setEnabled(!step);automationTab_.setEnabled(!automation);samplerTab_.setEnabled(!sampler);
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
    void updateBpmLabel(){bpmLabel_.setText("BPM "+juce::String(project_.transport.bpm,1),juce::dontSendNotification);}
    void changeBpm(double delta){Project before=project_;project_.transport.bpm=std::clamp(project_.transport.bpm+delta,20.0,300.0);undo_.commit(std::move(before),project_,"Change BPM");publishEdit("BPM "+juce::String(project_.transport.bpm,1));}
    void syncMixerControls(){
        const int index=selectedTrackIndex();suppressMixerCallbacks_=true;
        if(index<0){mixerVolume_.setValue(1.0,juce::dontSendNotification);mixerPan_.setValue(0.0,juce::dontSendNotification);muteTrack_.setButtonText("Mute");soloTrack_.setButtonText("Solo");trackMeter_.clear();}
        else{auto const&t=project_.tracks[static_cast<std::size_t>(index)];mixerVolume_.setValue(t.mixer.volume,juce::dontSendNotification);mixerPan_.setValue(t.mixer.pan,juce::dontSendNotification);muteTrack_.setButtonText(t.mixer.mute?"Muted":"Mute");soloTrack_.setButtonText(t.mixer.solo?"Soloed":"Solo");}
        suppressMixerCallbacks_=false;
    }
    void beginMixerGesture(){if(mixerGestureActive_)return;mixerBefore_=project_;mixerGestureActive_=true;}
    void applyMixerSliders(){
        if(suppressMixerCallbacks_)return;const int index=selectedTrackIndex();if(index<0)return;if(!mixerGestureActive_)beginMixerGesture();
        auto&t=project_.tracks[static_cast<std::size_t>(index)];t.mixer.volume=static_cast<float>(mixerVolume_.getValue());t.mixer.pan=static_cast<float>(mixerPan_.getValue());engine_.publish(project_);if(arrangement_)arrangement_->repaint();
    }
    void endMixerGesture(){if(!mixerGestureActive_)return;mixerGestureActive_=false;undo_.commit(std::move(mixerBefore_),project_,"Edit mixer channel");status_.setText("Mixer edit committed",juce::dontSendNotification);}
    void toggleMixerFlag(bool solo){
        const int index=selectedTrackIndex();if(index<0)return;Project before=project_;auto&t=project_.tracks[static_cast<std::size_t>(index)];if(solo)t.mixer.solo=!t.mixer.solo;else t.mixer.mute=!t.mixer.mute;undo_.commit(std::move(before),project_,solo?"Toggle track solo":"Toggle track mute");publishEdit(solo?"Track solo changed":"Track mute changed");
    }
    void assignSelectedInstrument(){
        const int trackIndex=selectedTrackIndex(),pluginIndex=pluginChoice_.getSelectedId()-1;
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
        const int pluginIndex=pluginChoice_.getSelectedId()-1;if(pluginIndex<0||pluginIndex>=static_cast<int>(plugins_.size())){status_.setText("Scan and select an effect first",juce::dontSendNotification);return;}
        const auto&descriptor=plugins_[static_cast<std::size_t>(pluginIndex)];if(descriptor.instrument){status_.setText("Selected plugin is an instrument, not an effect insert",juce::dontSendNotification);return;}
        if(!master&&selectedTrackIndex()<0){status_.setText("Select a track first",juce::dontSendNotification);return;}
        PluginInstance instance;instance.format=descriptor.format;instance.identifier=descriptor.identifier;instance.name=descriptor.name;instance.enabled=true;instance.bypass=false;instance.wet=1.0f;
        Project before=project_;if(master)project_.master.plugins.push_back(instance);else project_.tracks[static_cast<std::size_t>(selectedTrackIndex())].mixer.plugins.push_back(instance);
        undo_.commit(std::move(before),project_,master?"Add master insert":"Add track insert");publishEdit((master?"Master FX: ":"Track FX: ")+juce::String(descriptor.name));
    }
    void undoEdit(){if(!undo_.undo(project_)){status_.setText("Nothing to undo",juce::dontSendNotification);return;}publishEdit("Undo");}
    void redoEdit(){if(!undo_.redo(project_)){status_.setText("Nothing to redo",juce::dontSendNotification);return;}publishEdit("Redo");}
    void writeProject(std::filesystem::path path){
        try{if(path.extension()!=".flow")path+=".flow";ProjectSerializer::save(project_,path);projectPath_=path;settings_.lastProjectPath=path;projectLabel_.setText("Project: "+juce::String(path.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);status_.setText("Project saved (v11)",juce::dontSendNotification);}
        catch(const std::exception&e){status_.setText("Save failed: "+juce::String(e.what()),juce::dontSendNotification);}
    }
    void saveProject(){
        if(audioRecording_)finishAudioRecording();if(recordingChops_)finishChopRecording();
        if(!projectPath_.empty()){writeProject(projectPath_);return;}
        chooser_=std::make_unique<juce::FileChooser>("Save FLOWDAW project",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("FLOWDAW.flow"),"*.flow");
        chooser_->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser&fc){auto file=fc.getResult();if(file!=juce::File{})writeProject(std::filesystem::path(file.getFullPathName().toStdString()));chooser_.reset();});
    }
    juce::AudioPluginFormat* formatFor(const PluginDescriptor&p){for(auto*f:formatManager_.getFormats()){auto n=f->getName().toLowerCase();if((p.format=="vst3"&&n.contains("vst3"))||(p.format=="au"&&n.contains("audio")))return f;}return nullptr;}
    void openSelectedEditor(){const int idx=pluginChoice_.getSelectedId()-1;if(idx<0||idx>=static_cast<int>(plugins_.size())){status_.setText("Scan and select a plugin first",juce::dontSendNotification);return;}auto p=plugins_[static_cast<std::size_t>(idx)];if(safety_.isQuarantined(p.identifier)){status_.setText("Plugin is quarantined; clear it with FLOWDAW Doctor before retrying",juce::dontSendNotification);return;}auto*fmt=formatFor(p);if(!fmt){status_.setText("No JUCE format backend for selection",juce::dontSendNotification);return;}juce::OwnedArray<juce::PluginDescription> desc;fmt->findAllTypesForFile(desc,juce::String(p.identifier));if(desc.isEmpty()){safety_.noteFailure(p.identifier,"Plugin description could not be recreated");saveSafety();status_.setText("Plugin description could not be recreated",juce::dontSendNotification);return;}auto d=*desc[0];auto setup=deviceManager_.getAudioDeviceSetup();formatManager_.createPluginInstanceAsync(d,setup.sampleRate>0?setup.sampleRate:48000.0,setup.bufferSize>0?setup.bufferSize:256,[this,id=p.identifier](std::unique_ptr<juce::AudioPluginInstance>instance,const juce::String&error){if(!instance){safety_.noteFailure(id,error.toStdString());saveSafety();status_.setText("Plugin load failed: "+error,juce::dontSendNotification);return;}safety_.noteSuccess(id);saveSafety();pluginWindow_=std::make_unique<PluginEditorWindow>(std::move(instance));status_.setText("Plugin editor hosted in JUCE window",juce::dontSendNotification);});}
    void timerCallback()override{
        engine_.collectRetiredGraphs();
        play_.setButtonText(engine_.isPlaying()?"Pause":"Play");
        recChops_.setButtonText(recordingChops_?"STOP CHOPS":"REC CHOPS");recAudio_.setButtonText(audioRecording_?"STOP Audio":"REC Audio");
        if(arrangement_){const auto tick=MusicalTime::samplesToTicks(engine_.playheadSamples(),project_.transport.bpm,engine_.sampleRate());arrangement_->setPlayheadTick(tick);}
        const auto meters=engine_.meterSnapshot();
        const auto db=[](float value){return value>0.000001f?20.0f*std::log10(value):-120.0f;};
        meterLabel_.setText("Meters (TP est / sample / RMS) | Master L "
                            +juce::String(db(meters.master.truePeakLeft),1)+" / "+juce::String(db(meters.master.samplePeakLeft),1)+" / "+juce::String(db(meters.master.rmsLeft),1)
                            +" dB | R "+juce::String(db(meters.master.truePeakRight),1)+" / "+juce::String(db(meters.master.samplePeakRight),1)+" / "+juce::String(db(meters.master.rmsRight),1)
                            +" dB | "+juce::String(static_cast<int>(meters.tracks.size()))+" track meter(s), "
                            +juce::String(static_cast<int>(meters.buses.size()))+" bus meter(s)",juce::dontSendNotification);
        const int selected=selectedTrackIndex();bool found=false;if(selected>=0){const Id id=project_.tracks[static_cast<std::size_t>(selected)].id;for(auto const&route:meters.tracks)if(route.id==id){trackMeter_.setReading(route.level);found=true;break;}}if(!found)trackMeter_.clear();
        if(++settingsSaveTicks_>=50){settingsSaveTicks_=0;saveDeviceSettings();}
    }
    void saveSafety(){try{safety_.save(safetyPath_);}catch(...){} }
    void saveDeviceSettings(){auto s=deviceManager_.getAudioDeviceSetup();settings_.audio.preferredSampleRate=s.sampleRate>0?static_cast<int>(s.sampleRate):48000;settings_.audio.bufferSize=sanitizeBufferSize(static_cast<unsigned long>(std::max(1,s.bufferSize)));settings_.audio.inputDevice=s.inputDeviceName.toStdString();settings_.audio.outputDevice=s.outputDeviceName.toStdString();try{saveAppSettings(settings_,settingsPath_);}catch(...){} }
    AppSettings settings_;PluginSafetyRegistry safety_;std::filesystem::path settingsPath_,safetyPath_,projectPath_;Project project_;UndoStack undo_;AudioEngine engine_;std::shared_ptr<PluginHost>pluginHost_;juce::AudioDeviceManager deviceManager_;juce::AudioPluginFormatManager formatManager_;std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;std::unique_ptr<juce::FileChooser> chooser_;std::vector<PluginDescriptor> plugins_;std::unique_ptr<PluginEditorWindow> pluginWindow_;std::unique_ptr<juceui::ArrangementComponent> arrangement_;std::unique_ptr<juceui::PianoRollComponent> piano_;std::unique_ptr<juceui::SamplerComponent> sampler_;std::unique_ptr<juceui::StepSequencerComponent> sequencer_;std::unique_ptr<juceui::AutomationAssistComponent> automationAssist_;EditorMode editorMode_=EditorMode::Arrangement;std::array<float,kMaxDeviceBlock>monoInput_{};std::array<float,kMaxDeviceBlock*2>stereoOutput_{};juce::Label title_,status_,projectLabel_,note_,meterLabel_,bpmLabel_,mixerVolumeLabel_,mixerPanLabel_,trackMeterLabel_,rackParamLabel_;int settingsSaveTicks_=0;bool suppressMixerCallbacks_=false,mixerGestureActive_=false,suppressRackCallbacks_=false,rackGestureActive_=false,rackEditorDirty_=false,suppressChopCallbacks_=false,chopGestureActive_=false,recordingChops_=false,audioRecording_=false;Id chopRecordPatternId_=0,audioRecordTrackId_=0,rackEditorPluginId_=0;Tick chopRecordStartTick_=0,audioRecordStartTick_=0;Project mixerBefore_,rackBefore_,rackEditorBefore_,chopBefore_,chopRecordBefore_,audioRecordBefore_;juce::TextButton newProject_,loadProject_,importWav_,saveProject_,play_,stop_,bpmMinus_,bpmPlus_,undoButton_,redoButton_,scan_,openEditor_,setInstrument_,clearInstrument_,addTrackFx_,addMasterFx_,muteTrack_,soloTrack_,arrangementTab_,pianoTab_,sequencerTab_,automationTab_,samplerTab_,bankPrev_,bankNext_,analyzeSample_,chop8_,autoChop_,chopBeat_,chopBar_,matchBpm_,exportMix_,exportStems_,stopPreview_,recChops_,chopReset_,recAudio_,monitorInput_,prevTake_,nextTake_,renamePad_,padGainMinus_,padGainPlus_,padPanMinus_,padPanPlus_,padChokeMinus_,padChokePlus_,addRackGain_,addRackClip_,addRackWidth_,addRackExternal_,rackMoveUp_,rackMoveDown_,rackEnabled_,rackBypass_,rackRemove_,openRackEditor_;juce::ComboBox pluginChoice_,trackChoice_,patternChoice_,sampleChoice_,rackTargetChoice_,rackPluginChoice_,chopGridChoice_;juce::TextEditor padName_;juce::Slider mixerVolume_,mixerPan_,rackWet_,rackParam_,chopQuantizeStrength_,chopHumanizeStrength_;juceui::StereoMeterComponent trackMeter_;
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
