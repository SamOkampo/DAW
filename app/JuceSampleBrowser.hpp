#pragma once
#include "flowdaw/AppSettings.hpp"
#include <juce_gui_extra/juce_gui_extra.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace juceui {
class SampleBrowserComponent final:public juce::Component,private juce::ListBoxModel,public juce::FileDragAndDropTarget{
public:
    using ImportFn=std::function<void(const std::filesystem::path&)>;
    using SettingsChangedFn=std::function<void()>;

    SampleBrowserComponent(flowdaw::AppSettings&settings,ImportFn importFn,SettingsChangedFn settingsChanged)
        :settings_(settings),importFn_(std::move(importFn)),settingsChanged_(std::move(settingsChanged)),list_("Samples",this){
        title_.setText("SAMPLE BROWSER",juce::dontSendNotification);title_.setFont(juce::Font(14.0f,juce::Font::bold));addAndMakeVisible(title_);
        search_.setTextToShowWhenEmpty("Search WAVs...",juce::Colour(0xff7f8796));search_.onTextChange=[this]{refresh();};addAndMakeVisible(search_);
        source_.addItem("Library",1);source_.addItem("Favorites",2);source_.addItem("Recent",3);source_.setSelectedId(1,juce::dontSendNotification);source_.onChange=[this]{refresh();};addAndMakeVisible(source_);
        root_.setTextWhenNothingSelected("No sample folder");root_.onChange=[this]{refresh();};addAndMakeVisible(root_);
        addRoot_.setButtonText("+ Folder");addRoot_.onClick=[this]{chooseRoot();};addAndMakeVisible(addRoot_);
        favorite_.setButtonText("★ Favorite");favorite_.onClick=[this]{toggleFavorite();};addAndMakeVisible(favorite_);
        import_.setButtonText("Import");import_.onClick=[this]{importSelected();};addAndMakeVisible(import_);
        list_.setRowHeight(34);list_.setOutlineThickness(0);addAndMakeVisible(list_);
        hint_.setText("Double-click or drop WAVs to import",juce::dontSendNotification);hint_.setColour(juce::Label::textColourId,juce::Colour(0xff8d95a5));addAndMakeVisible(hint_);
        if(settings_.sampleRoots.empty()){
            auto music=juce::File::getSpecialLocation(juce::File::userMusicDirectory);
            auto fallback=juce::File::getSpecialLocation(juce::File::userHomeDirectory);
            auto chosen=music.isDirectory()?music:fallback;
            if(chosen.isDirectory()){settings_.sampleRoots.emplace_back(chosen.getFullPathName().toStdString());notifySettingsChanged();}
        }
        refreshRoots();refresh();
    }

    void paint(juce::Graphics&g)override{
        g.fillAll(juce::Colour(0xff11141a));g.setColour(juce::Colour(0xff2a2f38));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f),7.0f,1.0f);
    }
    void resized()override{
        auto r=getLocalBounds().reduced(8);title_.setBounds(r.removeFromTop(24));search_.setBounds(r.removeFromTop(30));
        auto row=r.removeFromTop(30);source_.setBounds(row.removeFromLeft(96).reduced(2));root_.setBounds(row.reduced(2));
        auto actions=r.removeFromTop(30);addRoot_.setBounds(actions.removeFromLeft(82).reduced(2));favorite_.setBounds(actions.removeFromLeft(96).reduced(2));import_.setBounds(actions.removeFromLeft(72).reduced(2));
        hint_.setBounds(r.removeFromBottom(22));list_.setBounds(r.reduced(2));
    }

    bool isInterestedInFileDrag(const juce::StringArray&files)override{
        for(auto const&file:files){juce::File f(file);if(f.isDirectory()||f.hasFileExtension("wav"))return true;}return false;
    }
    void filesDropped(const juce::StringArray&files,int,int)override{
        for(auto const&raw:files){
            juce::File f(raw);
            if(f.isDirectory()){addRootPath(std::filesystem::path(f.getFullPathName().toStdString()));continue;}
            if(f.existsAsFile()&&f.hasFileExtension("wav"))importPath(std::filesystem::path(f.getFullPathName().toStdString()));
        }
        refresh();
    }

private:
    int getNumRows()override{return static_cast<int>(visible_.size());}
    void paintListBoxItem(int row,juce::Graphics&g,int width,int height,bool selected)override{
        if(row<0||row>=static_cast<int>(visible_.size()))return;
        if(selected)g.fillAll(juce::Colour(0xff263249));
        const auto&p=visible_[static_cast<std::size_t>(row)];
        const bool fav=contains(settings_.favoriteSamples,p);
        g.setColour(juce::Colour(0xfff0f3f8));g.drawText((fav?"★ ":"")+juce::String(p.filename().string()),8,2,width-16,16,juce::Justification::centredLeft,true);
        g.setColour(juce::Colour(0xff7f8796));g.setFont(11.0f);g.drawText(juce::String(p.parent_path().string()),8,18,width-16,std::max(12,height-18),juce::Justification::centredLeft,true);
    }
    void listBoxItemDoubleClicked(int row,const juce::MouseEvent&)override{
        if(row>=0&&row<static_cast<int>(visible_.size()))importPath(visible_[static_cast<std::size_t>(row)]);
    }

    static bool contains(const std::vector<std::filesystem::path>&items,const std::filesystem::path&p){
        return std::find(items.begin(),items.end(),p)!=items.end();
    }
    static std::string lower(std::string s){for(char&c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
    bool matchesSearch(const std::filesystem::path&p)const{
        auto q=lower(search_.getText().trim().toStdString());if(q.empty())return true;auto hay=lower(p.filename().string()+" "+p.parent_path().string());return hay.find(q)!=std::string::npos;
    }
    void notifySettingsChanged(){if(settingsChanged_)settingsChanged_();}
    void refreshRoots(){
        const int previous=root_.getSelectedId();root_.clear(juce::dontSendNotification);int id=1;for(auto const&p:settings_.sampleRoots)root_.addItem(juce::String(p.string()),id++);
        if(!settings_.sampleRoots.empty())root_.setSelectedId(previous>0&&previous<=static_cast<int>(settings_.sampleRoots.size())?previous:1,juce::dontSendNotification);
    }
    void refresh(){
        visible_.clear();const int source=source_.getSelectedId();
        if(source==2){for(auto const&p:settings_.favoriteSamples)if(std::filesystem::exists(p)&&matchesSearch(p))visible_.push_back(p);}
        else if(source==3){for(auto const&p:settings_.recentSamples)if(std::filesystem::exists(p)&&matchesSearch(p))visible_.push_back(p);}
        else{
            const int idx=root_.getSelectedId()-1;if(idx>=0&&idx<static_cast<int>(settings_.sampleRoots.size())){
                const auto base=settings_.sampleRoots[static_cast<std::size_t>(idx)];std::error_code ec;std::size_t count=0;
                if(std::filesystem::exists(base,ec))for(std::filesystem::recursive_directory_iterator it(base,std::filesystem::directory_options::skip_permission_denied,ec),end;it!=end&&count<5000;it.increment(ec)){
                    if(ec){ec.clear();continue;}if(!it->is_regular_file(ec))continue;auto ext=lower(it->path().extension().string());if(ext!=".wav")continue;if(matchesSearch(it->path()))visible_.push_back(it->path());++count;
                }
                std::sort(visible_.begin(),visible_.end(),[](const auto&a,const auto&b){return lower(a.filename().string())<lower(b.filename().string());});
            }
        }
        list_.updateContent();list_.repaint();favorite_.setEnabled(!visible_.empty());import_.setEnabled(!visible_.empty());
    }
    void addRootPath(const std::filesystem::path&p){
        if(p.empty()||!std::filesystem::is_directory(p))return;if(!contains(settings_.sampleRoots,p)){settings_.sampleRoots.push_back(p);if(settings_.sampleRoots.size()>64)settings_.sampleRoots.erase(settings_.sampleRoots.begin());notifySettingsChanged();}
        refreshRoots();for(std::size_t i=0;i<settings_.sampleRoots.size();++i)if(settings_.sampleRoots[i]==p){root_.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);break;}source_.setSelectedId(1,juce::dontSendNotification);
    }
    void chooseRoot(){
        chooser_=std::make_unique<juce::FileChooser>("Add sample folder",juce::File::getSpecialLocation(juce::File::userMusicDirectory));
        chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[this](const juce::FileChooser&fc){auto f=fc.getResult();if(f.isDirectory())addRootPath(std::filesystem::path(f.getFullPathName().toStdString()));chooser_.reset();refresh();});
    }
    std::filesystem::path selectedPath()const{const int row=list_.getSelectedRow();return row>=0&&row<static_cast<int>(visible_.size())?visible_[static_cast<std::size_t>(row)]:std::filesystem::path{};}
    void rememberRecent(const std::filesystem::path&p){
        auto&v=settings_.recentSamples;v.erase(std::remove(v.begin(),v.end(),p),v.end());v.insert(v.begin(),p);if(v.size()>32)v.resize(32);notifySettingsChanged();
    }
    void importPath(const std::filesystem::path&p){if(p.empty()||!std::filesystem::exists(p))return;rememberRecent(p);if(importFn_)importFn_(p);refresh();}
    void importSelected(){auto p=selectedPath();if(!p.empty())importPath(p);}
    void toggleFavorite(){
        auto p=selectedPath();if(p.empty())return;auto&v=settings_.favoriteSamples;auto it=std::find(v.begin(),v.end(),p);if(it==v.end()){v.push_back(p);if(v.size()>256)v.erase(v.begin());}else v.erase(it);notifySettingsChanged();refresh();
    }

    flowdaw::AppSettings&settings_;ImportFn importFn_;SettingsChangedFn settingsChanged_;std::vector<std::filesystem::path>visible_;std::unique_ptr<juce::FileChooser>chooser_;
    juce::Label title_,hint_;juce::TextEditor search_;juce::ComboBox source_,root_;juce::TextButton addRoot_,favorite_,import_;juce::ListBox list_;
};
}
