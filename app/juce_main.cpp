#include "flowdaw/AppSettings.hpp"
#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/PluginSafety.hpp"
#include "flowdaw/Serialization.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
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
        play_.setButtonText("Play");play_.onClick=[this]{engine_.play();status_.setText("Playing through JUCE device",juce::dontSendNotification);};addAndMakeVisible(play_);
        stop_.setButtonText("Stop");stop_.onClick=[this]{engine_.stop();status_.setText("Stopped",juce::dontSendNotification);};addAndMakeVisible(stop_);
        scan_.setButtonText("Scan VST3/AU");scan_.onClick=[this]{scanPlugins();};addAndMakeVisible(scan_);
        openEditor_.setButtonText("Open Plugin Editor");openEditor_.onClick=[this]{openSelectedEditor();};addAndMakeVisible(openEditor_);
        addAndMakeVisible(pluginChoice_);pluginChoice_.setTextWhenNothingSelected("No plugin selected");
        note_.setText("JUCE owns device I/O and registers the real VST3/AU backend with FLOWDAW's AudioEngine. Phase 8 now executes prepared track, bus and master inserts through preallocated realtime route buffers with PDC; full Studio editing parity remains in progress.",juce::dontSendNotification);note_.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(note_);
        setSize(1100,760);startTimer(5000);
    }
    ~MainComponent()override{deviceManager_.removeAudioCallback(this);engine_.stop();engine_.collectRetiredGraphs();saveDeviceSettings();saveSafety();}
    void resized()override{
        auto r=getLocalBounds().reduced(16);title_.setBounds(r.removeFromTop(38));status_.setBounds(r.removeFromTop(26));projectLabel_.setBounds(r.removeFromTop(26));note_.setBounds(r.removeFromTop(54));auto transport=r.removeFromTop(38);loadProject_.setBounds(transport.removeFromLeft(130).reduced(3));play_.setBounds(transport.removeFromLeft(80).reduced(3));stop_.setBounds(transport.removeFromLeft(80).reduced(3));auto controls=r.removeFromTop(38);scan_.setBounds(controls.removeFromLeft(150).reduced(3));pluginChoice_.setBounds(controls.removeFromLeft(430).reduced(3));openEditor_.setBounds(controls.removeFromLeft(180).reduced(3));r.removeFromTop(8);selector_->setBounds(r);
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
    void loadProjectFile(const std::filesystem::path&p){try{engine_.stop();project_=ProjectSerializer::load(p,true);projectPath_=p;settings_.lastProjectPath=p;engine_.publish(project_);projectLabel_.setText("Project: "+juce::String(p.filename().string())+" | "+juce::String(static_cast<int>(project_.tracks.size()))+" tracks",juce::dontSendNotification);status_.setText("Project loaded; Play uses JUCE device callback",juce::dontSendNotification);}catch(const std::exception&e){status_.setText("Open failed: "+juce::String(e.what()),juce::dontSendNotification);}}
    std::vector<std::filesystem::path> pluginRoots()const{
        if(!settings_.pluginRoots.empty())return settings_.pluginRoots;std::vector<std::filesystem::path> roots;auto home=std::filesystem::path(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName().toStdString());roots.push_back(home/".vst3");roots.push_back("/usr/lib/vst3");roots.push_back("/usr/local/lib/vst3");
#if JUCE_MAC
        roots.push_back(home/"Library/Audio/Plug-Ins/VST3");roots.push_back(home/"Library/Audio/Plug-Ins/Components");roots.push_back("/Library/Audio/Plug-Ins/VST3");roots.push_back("/Library/Audio/Plug-Ins/Components");
#endif
        return roots;
    }
    void scanPlugins(){std::string error;plugins_=scanPluginsWithJuce(pluginRoots(),error,&safety_);saveSafety();pluginChoice_.clear(juce::dontSendNotification);int id=1;for(auto const&p:plugins_)pluginChoice_.addItem(juce::String(p.name+" ["+p.format+"]"),id++);if(!plugins_.empty())pluginChoice_.setSelectedId(1,juce::dontSendNotification);int quarantined=0;for(auto const&r:safety_.records())if(r.quarantined)++quarantined;auto base="JUCE scan: "+juce::String(static_cast<int>(plugins_.size()))+" plugin type(s), "+juce::String(quarantined)+" quarantined";status_.setText(error.empty()?base:base+" | "+juce::String(error),juce::dontSendNotification);}
    juce::AudioPluginFormat* formatFor(const PluginDescriptor&p){for(auto*f:formatManager_.getFormats()){auto n=f->getName().toLowerCase();if((p.format=="vst3"&&n.contains("vst3"))||(p.format=="au"&&n.contains("audio")))return f;}return nullptr;}
    void openSelectedEditor(){const int idx=pluginChoice_.getSelectedId()-1;if(idx<0||idx>=static_cast<int>(plugins_.size())){status_.setText("Scan and select a plugin first",juce::dontSendNotification);return;}auto p=plugins_[static_cast<std::size_t>(idx)];if(safety_.isQuarantined(p.identifier)){status_.setText("Plugin is quarantined; clear it with FLOWDAW Doctor before retrying",juce::dontSendNotification);return;}auto*fmt=formatFor(p);if(!fmt){status_.setText("No JUCE format backend for selection",juce::dontSendNotification);return;}juce::OwnedArray<juce::PluginDescription> desc;fmt->findAllTypesForFile(desc,juce::String(p.identifier));if(desc.isEmpty()){safety_.noteFailure(p.identifier,"Plugin description could not be recreated");saveSafety();status_.setText("Plugin description could not be recreated",juce::dontSendNotification);return;}auto d=*desc[0];auto setup=deviceManager_.getAudioDeviceSetup();formatManager_.createPluginInstanceAsync(d,setup.sampleRate>0?setup.sampleRate:48000.0,setup.bufferSize>0?setup.bufferSize:256,[this,id=p.identifier](std::unique_ptr<juce::AudioPluginInstance>instance,const juce::String&error){if(!instance){safety_.noteFailure(id,error.toStdString());saveSafety();status_.setText("Plugin load failed: "+error,juce::dontSendNotification);return;}safety_.noteSuccess(id);saveSafety();pluginWindow_=std::make_unique<PluginEditorWindow>(std::move(instance));status_.setText("Plugin editor hosted in JUCE window",juce::dontSendNotification);});}
    void timerCallback()override{engine_.collectRetiredGraphs();saveDeviceSettings();}
    void saveSafety(){try{safety_.save(safetyPath_);}catch(...){} }
    void saveDeviceSettings(){auto s=deviceManager_.getAudioDeviceSetup();settings_.audio.preferredSampleRate=s.sampleRate>0?static_cast<int>(s.sampleRate):48000;settings_.audio.bufferSize=sanitizeBufferSize(static_cast<unsigned long>(std::max(1,s.bufferSize)));settings_.audio.inputDevice=s.inputDeviceName.toStdString();settings_.audio.outputDevice=s.outputDeviceName.toStdString();try{saveAppSettings(settings_,settingsPath_);}catch(...){} }
    AppSettings settings_;PluginSafetyRegistry safety_;std::filesystem::path settingsPath_,safetyPath_,projectPath_;Project project_;AudioEngine engine_;std::shared_ptr<PluginHost>pluginHost_;juce::AudioDeviceManager deviceManager_;juce::AudioPluginFormatManager formatManager_;std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;std::unique_ptr<juce::FileChooser> chooser_;std::vector<PluginDescriptor> plugins_;std::unique_ptr<PluginEditorWindow> pluginWindow_;std::array<float,kMaxDeviceBlock>monoInput_{};std::array<float,kMaxDeviceBlock*2>stereoOutput_{};juce::Label title_,status_,projectLabel_,note_;juce::TextButton loadProject_,play_,stop_,scan_,openEditor_;juce::ComboBox pluginChoice_;
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
