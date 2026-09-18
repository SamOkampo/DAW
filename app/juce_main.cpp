#include "flowdaw/AppSettings.hpp"
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/JucePluginBackend.hpp"
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

class PluginEditorWindow final:public juce::DocumentWindow{
public:
    explicit PluginEditorWindow(std::unique_ptr<juce::AudioPluginInstance> p)
        :DocumentWindow("FLOWDAW Plugin - "+p->getName(),juce::Colours::darkgrey,DocumentWindow::closeButton),plugin_(std::move(p)){
        auto*editor=plugin_->createEditorIfNeeded();if(editor==nullptr)editor=new juce::GenericAudioProcessorEditor(*plugin_);
        setUsingNativeTitleBar(true);setResizable(true,true);setContentOwned(editor,true);centreWithSize(std::max(420,getWidth()),std::max(300,getHeight()));setVisible(true);
    }
    void closeButtonPressed()override{setVisible(false);}
private:std::unique_ptr<juce::AudioPluginInstance> plugin_;
};

class MainComponent final:public juce::Component,private juce::Timer,private juce::AudioIODeviceCallback{
public:
    MainComponent(){
        pluginHost_=std::make_shared<PluginHost>();pluginHost_->registerBackend(makeJucePluginBackend());engine_.setPluginHost(pluginHost_);
        const auto config=defaultSettingsDirectory();settingsPath_=config/"settings.conf";safetyPath_=config/"plugin-safety.conf";settings_=std::filesystem::exists(settingsPath_)?loadAppSettings(settingsPath_):defaultAppSettings();safety_.load(safetyPath_);
        if(!settings_.lastProjectPath.empty()&&std::filesystem::exists(settings_.lastProjectPath))try{project_=ProjectSerializer::load(settings_.lastProjectPath,true);projectPath_=settings_.lastProjectPath;}catch(...){}
        juce::String err=deviceManager_.initialise(2,2,nullptr,true);auto setup=deviceManager_.getAudioDeviceSetup();setup.sampleRate=settings_.audio.preferredSampleRate;setup.bufferSize=static_cast<int>(sanitizeBufferSize(settings_.audio.bufferSize));
        if(!settings_.audio.inputDevice.empty())setup.inputDeviceName=settings_.audio.inputDevice;if(!settings_.audio.outputDevice.empty())setup.outputDeviceName=settings_.audio.outputDevice;deviceManager_.setAudioDeviceSetup(setup,true);
        selector_=std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager_,0,2,0,2,true,true,true,false);addAndMakeVisible(*selector_);deviceManager_.addAudioCallback(this);
        addHostFormats(formatManager_);
        title_.setText("FLOWDAW JUCE Runtime",juce::dontSendNotification);title_.setFont(juce::Font(24.0f,juce::Font::bold));addAndMakeVisible(title_);
        status_.setText(err.isEmpty()?"JUCE AudioDeviceManager drives FLOWDAW core":"Audio: "+err,juce::dontSendNotification);addAndMakeVisible(status_);
        projectLabel_.setText(projectPath_.empty()?"No project loaded":"Project: "+juce::String(projectPath_.filename().string()),juce::dontSendNotification);addAndMakeVisible(projectLabel_);
        loadProject_.setButtonText("Open .flow");loadProject_.onClick=[this]{chooseProject();};addAndMakeVisible(loadProject_);
        play_.setButtonText("Play");play_.onClick=[this]{if(engine_.isPlaying()){engine_.pause();status_.setText("Paused",juce::dontSendNotification);}else{engine_.play();status_.setText("Playing through JUCE device",juce::dontSendNotification);}};addAndMakeVisible(play_);
        bpmMinus_.setButtonText("- BPM");bpmMinus_.onClick=[this]{changeBpm(-1.0);};addAndMakeVisible(bpmMinus_);
        bpmPlus_.setButtonText("+ BPM");bpmPlus_.onClick=[this]{changeBpm(1.0);};addAndMakeVisible(bpmPlus_);
        bpmLabel_.setJustificationType(juce::Justification::centred);addAndMakeVisible(bpmLabel_);
        stop_.setButtonText("Stop");stop_.onClick=[this]{engine_.stop();status_.setText("Stopped",juce::dontSendNotification);};addAndMakeVisible(stop_);
        scan_.setButtonText("Scan VST3/AU");scan_.onClick=[this]{scanPlugins();};addAndMakeVisible(scan_);
        openEditor_.setButtonText("Open Plugin Editor");openEditor_.onClick=[this]{openSelectedEditor();};addAndMakeVisible(openEditor_);
        addAndMakeVisible(pluginChoice_);pluginChoice_.setTextWhenNothingSelected("No plugin selected");
        addAndMakeVisible(trackChoice_);trackChoice_.setTextWhenNothingSelected("No track selected");
        setInstrument_.setButtonText("Set Instrument");setInstrument_.onClick=[this]{assignSelectedInstrument();};addAndMakeVisible(setInstrument_);
        clearInstrument_.setButtonText("Clear Instrument");clearInstrument_.onClick=[this]{clearSelectedInstrument();};addAndMakeVisible(clearInstrument_);
        saveProject_.setButtonText("Save Project");saveProject_.onClick=[this]{saveProject();};addAndMakeVisible(saveProject_);
        undoButton_.setButtonText("Undo");undoButton_.onClick=[this]{undoEdit();};addAndMakeVisible(undoButton_);
        redoButton_.setButtonText("Redo");redoButton_.onClick=[this]{redoEdit();};addAndMakeVisible(redoButton_);
        trackChoice_.onChange=[this]{syncMixerControls();};
        mixerVolume_.setRange(0.0,2.0,0.01);mixerVolume_.setSliderStyle(juce::Slider::LinearHorizontal);mixerVolume_.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);mixerVolume_.onDragStart=[this]{beginMixerGesture();};mixerVolume_.onValueChange=[this]{applyMixerSliders();};mixerVolume_.onDragEnd=[this]{endMixerGesture();};addAndMakeVisible(mixerVolume_);
        mixerPan_.setRange(-1.0,1.0,0.01);mixerPan_.setSliderStyle(juce::Slider::LinearHorizontal);mixerPan_.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);mixerPan_.onDragStart=[this]{beginMixerGesture();};mixerPan_.onValueChange=[this]{applyMixerSliders();};mixerPan_.onDragEnd=[this]{endMixerGesture();};addAndMakeVisible(mixerPan_);
        mixerVolumeLabel_.setText("Track Volume",juce::dontSendNotification);mixerPanLabel_.setText("Pan",juce::dontSendNotification);addAndMakeVisible(mixerVolumeLabel_);addAndMakeVisible(mixerPanLabel_);
        muteTrack_.setButtonText("Mute");muteTrack_.onClick=[this]{toggleMixerFlag(false);};addAndMakeVisible(muteTrack_);
        soloTrack_.setButtonText("Solo");soloTrack_.onClick=[this]{toggleMixerFlag(true);};addAndMakeVisible(soloTrack_);
        trackMeterLabel_.setText("Selected track meter",juce::dontSendNotification);addAndMakeVisible(trackMeterLabel_);addAndMakeVisible(trackMeter_);
        auto commitEdit=[this](Project before,std::string name){undo_.commit(std::move(before),project_,name);publishEdit(juce::String(name));};
        arrangement_=std::make_unique<juceui::ArrangementComponent>(project_,commitEdit);addAndMakeVisible(*arrangement_);
        piano_=std::make_unique<juceui::PianoRollComponent>(project_,commitEdit);addChildComponent(*piano_);
        sampler_=std::make_unique<juceui::SamplerComponent>(project_,engine_,commitEdit);addChildComponent(*sampler_);
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
        refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();updateBpmLabel();setEditorMode(EditorMode::Arrangement);
        note_.setText("JUCE owns device I/O and registers the real VST3/AU backend with FLOWDAW's AudioEngine. Phase 8 now executes prepared track, bus and master inserts through preallocated realtime route buffers with PDC; full Studio editing parity remains in progress.",juce::dontSendNotification);note_.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(note_);
        meterLabel_.setText("Meters (TP estimate / sample peak / RMS): waiting for audio",juce::dontSendNotification);addAndMakeVisible(meterLabel_);
        setSize(1280,980);startTimer(100);
    }
    ~MainComponent()override{deviceManager_.removeAudioCallback(this);engine_.stop();engine_.collectRetiredGraphs();saveDeviceSettings();saveSafety();}
    void resized()override{
        auto r=getLocalBounds().reduced(16);title_.setBounds(r.removeFromTop(38));status_.setBounds(r.removeFromTop(26));projectLabel_.setBounds(r.removeFromTop(26));note_.setBounds(r.removeFromTop(54));meterLabel_.setBounds(r.removeFromTop(28));
        auto transport=r.removeFromTop(38);loadProject_.setBounds(transport.removeFromLeft(120).reduced(3));saveProject_.setBounds(transport.removeFromLeft(120).reduced(3));play_.setBounds(transport.removeFromLeft(76).reduced(3));stop_.setBounds(transport.removeFromLeft(76).reduced(3));bpmMinus_.setBounds(transport.removeFromLeft(68).reduced(3));bpmLabel_.setBounds(transport.removeFromLeft(90).reduced(3));bpmPlus_.setBounds(transport.removeFromLeft(68).reduced(3));undoButton_.setBounds(transport.removeFromLeft(76).reduced(3));redoButton_.setBounds(transport.removeFromLeft(76).reduced(3));
        auto controls=r.removeFromTop(38);scan_.setBounds(controls.removeFromLeft(150).reduced(3));pluginChoice_.setBounds(controls.removeFromLeft(430).reduced(3));openEditor_.setBounds(controls.removeFromLeft(180).reduced(3));
        auto instrument=r.removeFromTop(38);trackChoice_.setBounds(instrument.removeFromLeft(360).reduced(3));setInstrument_.setBounds(instrument.removeFromLeft(145).reduced(3));clearInstrument_.setBounds(instrument.removeFromLeft(155).reduced(3));
        auto editorBar=r.removeFromTop(38);arrangementTab_.setBounds(editorBar.removeFromLeft(105).reduced(3));pianoTab_.setBounds(editorBar.removeFromLeft(95).reduced(3));sequencerTab_.setBounds(editorBar.removeFromLeft(95).reduced(3));automationTab_.setBounds(editorBar.removeFromLeft(100).reduced(3));samplerTab_.setBounds(editorBar.removeFromLeft(85).reduced(3));patternChoice_.setBounds(editorBar.removeFromLeft(245).reduced(3));sampleChoice_.setBounds(editorBar.removeFromLeft(255).reduced(3));bankPrev_.setBounds(editorBar.removeFromLeft(72).reduced(3));bankNext_.setBounds(editorBar.removeFromLeft(72).reduced(3));
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
    void chooseProject(){chooser_=std::make_unique<juce::FileChooser>("Open FLOWDAW project",juce::File(projectPath_.string()),"*.flow");chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser&fc){auto f=fc.getResult();if(f.existsAsFile())loadProjectFile(std::filesystem::path(f.getFullPathName().toStdString()));chooser_.reset();});}
    void loadProjectFile(const std::filesystem::path&p){try{engine_.stop();project_=ProjectSerializer::load(p,true);projectPath_=p;settings_.lastProjectPath=p;undo_=UndoStack{};engine_.publish(project_);refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();projectLabel_.setText("Project: "+juce::String(p.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);status_.setText("Project loaded; Play uses JUCE device callback",juce::dontSendNotification);}catch(const std::exception&e){status_.setText("Open failed: "+juce::String(e.what()),juce::dontSendNotification);}}
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
    void publishEdit(const juce::String&message){engine_.publish(project_);refreshTrackChoice();refreshPatternChoice();refreshSampleChoice();syncMixerControls();updateBpmLabel();if(arrangement_)arrangement_->repaint();if(piano_)piano_->repaint();if(sampler_)sampler_->repaint();if(sequencer_)sequencer_->repaint();if(automationAssist_)automationAssist_->refresh();status_.setText(message,juce::dontSendNotification);}
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
    void undoEdit(){if(!undo_.undo(project_)){status_.setText("Nothing to undo",juce::dontSendNotification);return;}publishEdit("Undo");}
    void redoEdit(){if(!undo_.redo(project_)){status_.setText("Nothing to redo",juce::dontSendNotification);return;}publishEdit("Redo");}
    void writeProject(std::filesystem::path path){
        try{if(path.extension()!=".flow")path+=".flow";ProjectSerializer::save(project_,path);projectPath_=path;settings_.lastProjectPath=path;projectLabel_.setText("Project: "+juce::String(path.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);status_.setText("Project saved (v11)",juce::dontSendNotification);}
        catch(const std::exception&e){status_.setText("Save failed: "+juce::String(e.what()),juce::dontSendNotification);}
    }
    void saveProject(){
        if(!projectPath_.empty()){writeProject(projectPath_);return;}
        chooser_=std::make_unique<juce::FileChooser>("Save FLOWDAW project",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("FLOWDAW.flow"),"*.flow");
        chooser_->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser&fc){auto file=fc.getResult();if(file!=juce::File{})writeProject(std::filesystem::path(file.getFullPathName().toStdString()));chooser_.reset();});
    }
    juce::AudioPluginFormat* formatFor(const PluginDescriptor&p){for(auto*f:formatManager_.getFormats()){auto n=f->getName().toLowerCase();if((p.format=="vst3"&&n.contains("vst3"))||(p.format=="au"&&n.contains("audio")))return f;}return nullptr;}
    void openSelectedEditor(){const int idx=pluginChoice_.getSelectedId()-1;if(idx<0||idx>=static_cast<int>(plugins_.size())){status_.setText("Scan and select a plugin first",juce::dontSendNotification);return;}auto p=plugins_[static_cast<std::size_t>(idx)];if(safety_.isQuarantined(p.identifier)){status_.setText("Plugin is quarantined; clear it with FLOWDAW Doctor before retrying",juce::dontSendNotification);return;}auto*fmt=formatFor(p);if(!fmt){status_.setText("No JUCE format backend for selection",juce::dontSendNotification);return;}juce::OwnedArray<juce::PluginDescription> desc;fmt->findAllTypesForFile(desc,juce::String(p.identifier));if(desc.isEmpty()){safety_.noteFailure(p.identifier,"Plugin description could not be recreated");saveSafety();status_.setText("Plugin description could not be recreated",juce::dontSendNotification);return;}auto d=*desc[0];auto setup=deviceManager_.getAudioDeviceSetup();formatManager_.createPluginInstanceAsync(d,setup.sampleRate>0?setup.sampleRate:48000.0,setup.bufferSize>0?setup.bufferSize:256,[this,id=p.identifier](std::unique_ptr<juce::AudioPluginInstance>instance,const juce::String&error){if(!instance){safety_.noteFailure(id,error.toStdString());saveSafety();status_.setText("Plugin load failed: "+error,juce::dontSendNotification);return;}safety_.noteSuccess(id);saveSafety();pluginWindow_=std::make_unique<PluginEditorWindow>(std::move(instance));status_.setText("Plugin editor hosted in JUCE window",juce::dontSendNotification);});}
    void timerCallback()override{
        engine_.collectRetiredGraphs();
        play_.setButtonText(engine_.isPlaying()?"Pause":"Play");
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
    AppSettings settings_;PluginSafetyRegistry safety_;std::filesystem::path settingsPath_,safetyPath_,projectPath_;Project project_;UndoStack undo_;AudioEngine engine_;std::shared_ptr<PluginHost>pluginHost_;juce::AudioDeviceManager deviceManager_;juce::AudioPluginFormatManager formatManager_;std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;std::unique_ptr<juce::FileChooser> chooser_;std::vector<PluginDescriptor> plugins_;std::unique_ptr<PluginEditorWindow> pluginWindow_;std::unique_ptr<juceui::ArrangementComponent> arrangement_;std::unique_ptr<juceui::PianoRollComponent> piano_;std::unique_ptr<juceui::SamplerComponent> sampler_;std::unique_ptr<juceui::StepSequencerComponent> sequencer_;std::unique_ptr<juceui::AutomationAssistComponent> automationAssist_;EditorMode editorMode_=EditorMode::Arrangement;std::array<float,kMaxDeviceBlock>monoInput_{};std::array<float,kMaxDeviceBlock*2>stereoOutput_{};juce::Label title_,status_,projectLabel_,note_,meterLabel_,bpmLabel_,mixerVolumeLabel_,mixerPanLabel_,trackMeterLabel_;int settingsSaveTicks_=0;bool suppressMixerCallbacks_=false,mixerGestureActive_=false;Project mixerBefore_;juce::TextButton loadProject_,saveProject_,play_,stop_,bpmMinus_,bpmPlus_,undoButton_,redoButton_,scan_,openEditor_,setInstrument_,clearInstrument_,muteTrack_,soloTrack_,arrangementTab_,pianoTab_,sequencerTab_,automationTab_,samplerTab_,bankPrev_,bankNext_;juce::ComboBox pluginChoice_,trackChoice_,patternChoice_,sampleChoice_;juce::Slider mixerVolume_,mixerPan_;juceui::StereoMeterComponent trackMeter_;
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
