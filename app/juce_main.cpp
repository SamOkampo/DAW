#include "flowdaw/AppSettings.hpp"
#include "flowdaw/JucePluginBackend.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <filesystem>
#include <memory>
#include <vector>

namespace {
using namespace flowdaw;

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

class MainComponent final:public juce::Component,private juce::Timer{
public:
    MainComponent(){
        settingsPath_=defaultSettingsDirectory()/"settings.conf";settings_=std::filesystem::exists(settingsPath_)?loadAppSettings(settingsPath_):defaultAppSettings();
        juce::String err=deviceManager_.initialise(2,2,nullptr,true);auto setup=deviceManager_.getAudioDeviceSetup();setup.sampleRate=settings_.audio.preferredSampleRate;setup.bufferSize=static_cast<int>(sanitizeBufferSize(settings_.audio.bufferSize));
        if(!settings_.audio.inputDevice.empty())setup.inputDeviceName=settings_.audio.inputDevice;if(!settings_.audio.outputDevice.empty())setup.outputDeviceName=settings_.audio.outputDevice;deviceManager_.setAudioDeviceSetup(setup,true);
        selector_=std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager_,0,2,0,2,true,true,true,false);addAndMakeVisible(*selector_);
        addHostFormats(formatManager_);
        title_.setText("FLOWDAW JUCE Runtime",juce::dontSendNotification);title_.setFont(juce::Font(24.0f,juce::Font::bold));addAndMakeVisible(title_);
        status_.setText(err.isEmpty()?"JUCE AudioDeviceManager active":"Audio: "+err,juce::dontSendNotification);addAndMakeVisible(status_);
        scan_.setButtonText("Scan VST3/AU");scan_.onClick=[this]{scanPlugins();};addAndMakeVisible(scan_);
        openEditor_.setButtonText("Open Plugin Editor");openEditor_.onClick=[this]{openSelectedEditor();};addAndMakeVisible(openEditor_);
        addAndMakeVisible(pluginChoice_);pluginChoice_.setTextWhenNothingSelected("No plugin selected");
        note_.setText("Phase 7 production runtime: JUCE devices + real VST3/AU instantiation. The legacy X11 Studio remains available while editing views migrate.",juce::dontSendNotification);note_.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(note_);
        setSize(1040,720);startTimer(5000);
    }
    ~MainComponent()override{saveDeviceSettings();}
    void resized()override{
        auto r=getLocalBounds().reduced(16);title_.setBounds(r.removeFromTop(38));status_.setBounds(r.removeFromTop(28));note_.setBounds(r.removeFromTop(48));auto controls=r.removeFromTop(38);scan_.setBounds(controls.removeFromLeft(150).reduced(3));pluginChoice_.setBounds(controls.removeFromLeft(430).reduced(3));openEditor_.setBounds(controls.removeFromLeft(180).reduced(3));r.removeFromTop(8);selector_->setBounds(r);
    }
private:
    std::vector<std::filesystem::path> pluginRoots()const{
        if(!settings_.pluginRoots.empty())return settings_.pluginRoots;std::vector<std::filesystem::path> roots;auto home=std::filesystem::path(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName().toStdString());
        roots.push_back(home/".vst3");roots.push_back("/usr/lib/vst3");roots.push_back("/usr/local/lib/vst3");
#if JUCE_MAC
        roots.push_back(home/"Library/Audio/Plug-Ins/VST3");roots.push_back(home/"Library/Audio/Plug-Ins/Components");roots.push_back("/Library/Audio/Plug-Ins/VST3");roots.push_back("/Library/Audio/Plug-Ins/Components");
#endif
        return roots;
    }
    void scanPlugins(){
        std::string error;plugins_=scanPluginsWithJuce(pluginRoots(),error);pluginChoice_.clear(juce::dontSendNotification);int id=1;for(auto const&p:plugins_)pluginChoice_.addItem(juce::String(p.name+" ["+p.format+"]"),id++);if(!plugins_.empty())pluginChoice_.setSelectedId(1,juce::dontSendNotification);status_.setText(error.empty()?"JUCE scan: "+juce::String(static_cast<int>(plugins_.size()))+" plugin type(s)":"Scan error: "+juce::String(error),juce::dontSendNotification);
    }
    juce::AudioPluginFormat* formatFor(const PluginDescriptor&p){for(auto*f:formatManager_.getFormats()){auto n=f->getName().toLowerCase();if((p.format=="vst3"&&n.contains("vst3"))||(p.format=="au"&&n.contains("audio")))return f;}return nullptr;}
    void openSelectedEditor(){
        const int idx=pluginChoice_.getSelectedId()-1;if(idx<0||idx>=static_cast<int>(plugins_.size())){status_.setText("Scan and select a plugin first",juce::dontSendNotification);return;}auto p=plugins_[static_cast<std::size_t>(idx)];auto*fmt=formatFor(p);if(!fmt){status_.setText("No JUCE format backend for selection",juce::dontSendNotification);return;}juce::OwnedArray<juce::PluginDescription> desc;fmt->findAllTypesForFile(desc,juce::String(p.identifier));if(desc.isEmpty()){status_.setText("Plugin description could not be recreated",juce::dontSendNotification);return;}auto d=*desc[0];auto setup=deviceManager_.getAudioDeviceSetup();formatManager_.createPluginInstanceAsync(d,setup.sampleRate>0?setup.sampleRate:48000.0,setup.bufferSize>0?setup.bufferSize:256,[this](std::unique_ptr<juce::AudioPluginInstance>instance,const juce::String&error){if(!instance){status_.setText("Plugin load failed: "+error,juce::dontSendNotification);return;}pluginWindow_=std::make_unique<PluginEditorWindow>(std::move(instance));status_.setText("Plugin editor hosted in JUCE window",juce::dontSendNotification);});
    }
    void timerCallback()override{saveDeviceSettings();}
    void saveDeviceSettings(){auto s=deviceManager_.getAudioDeviceSetup();settings_.audio.preferredSampleRate=s.sampleRate>0?static_cast<int>(s.sampleRate):48000;settings_.audio.bufferSize=sanitizeBufferSize(static_cast<unsigned long>(std::max(1,s.bufferSize)));settings_.audio.inputDevice=s.inputDeviceName.toStdString();settings_.audio.outputDevice=s.outputDeviceName.toStdString();try{saveAppSettings(settings_,settingsPath_);}catch(...){} }
    AppSettings settings_;std::filesystem::path settingsPath_;juce::AudioDeviceManager deviceManager_;juce::AudioPluginFormatManager formatManager_;std::unique_ptr<juce::AudioDeviceSelectorComponent> selector_;std::vector<PluginDescriptor> plugins_;std::unique_ptr<PluginEditorWindow> pluginWindow_;juce::Label title_,status_,note_;juce::TextButton scan_,openEditor_;juce::ComboBox pluginChoice_;
};

class MainWindow final:public juce::DocumentWindow{
public:MainWindow():DocumentWindow("FLOWDAW",juce::Colours::black,DocumentWindow::allButtons){setUsingNativeTitleBar(true);setContentOwned(new MainComponent(),true);centreWithSize(getWidth(),getHeight());setVisible(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
};

class FlowdawApplication final:public juce::JUCEApplication{
public:const juce::String getApplicationName()override{return"FLOWDAW";}const juce::String getApplicationVersion()override{return"0.7.0";}bool moreThanOneInstanceAllowed()override{return true;}void initialise(const juce::String&)override{window_=std::make_unique<MainWindow>();}void shutdown()override{window_.reset();}void systemRequestedQuit()override{quit();}void anotherInstanceStarted(const juce::String&)override{}
private:std::unique_ptr<MainWindow> window_;
};
}
START_JUCE_APPLICATION(FlowdawApplication)
