#pragma once
#include "flowdaw/AppSettings.hpp"
#include "flowdaw/Wav.hpp"
#include <juce_gui_extra/juce_gui_extra.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace flowdaw::juceui {
class SampleBrowserComponent final : public juce::Component, private juce::ListBoxModel, private juce::KeyListener {
public:
    using ImportFn = std::function<void(const std::filesystem::path&)>;
    using PreviewFn = std::function<void(std::shared_ptr<AudioBuffer>)>;
    using StopPreviewFn = std::function<void()>;
    using SettingsChangedFn = std::function<void()>;

    SampleBrowserComponent(AppSettings& settings, ImportFn importFn, PreviewFn previewFn, StopPreviewFn stopPreviewFn, SettingsChangedFn settingsChanged)
        : settings_(settings), importFn_(std::move(importFn)), previewFn_(std::move(previewFn)), stopPreviewFn_(std::move(stopPreviewFn)), settingsChanged_(std::move(settingsChanged)), list_("Samples", this), previewPool_(1) {
        title_.setText("SAMPLE BROWSER", juce::dontSendNotification);
        title_.setFont(juce::Font(14.0f, juce::Font::bold)); addAndMakeVisible(title_);
        search_.setTextToShowWhenEmpty("Search WAVs...", juce::Colour(0xff7f8796));
        search_.onTextChange = [this] { refresh(); }; addAndMakeVisible(search_);
        root_.setTextWhenNothingSelected("No sample folder"); root_.onChange = [this] { showRecent_=false; showFavorites_=false; recent_.setToggleState(false, juce::dontSendNotification); favoritesOnly_.setToggleState(false, juce::dontSendNotification); refresh(); }; addAndMakeVisible(root_);
        addRoot_.setButtonText("+ Folder"); addRoot_.onClick = [this] { chooseRoot(); }; addAndMakeVisible(addRoot_);
        favorite_.setButtonText("Fav"); favorite_.onClick = [this] { toggleFavoriteSelected(); }; addAndMakeVisible(favorite_);
        recent_.setButtonText("Recent"); recent_.setClickingTogglesState(true); recent_.onClick = [this] { showRecent_=recent_.getToggleState(); if(showRecent_){showFavorites_=false;favoritesOnly_.setToggleState(false,juce::dontSendNotification);} refresh(); }; addAndMakeVisible(recent_);
        favoritesOnly_.setButtonText("Favorites"); favoritesOnly_.setClickingTogglesState(true); favoritesOnly_.onClick=[this]{showFavorites_=favoritesOnly_.getToggleState();if(showFavorites_){showRecent_=false;recent_.setToggleState(false,juce::dontSendNotification);}refresh();};addAndMakeVisible(favoritesOnly_);
        autoPreview_.setButtonText("Auto"); autoPreview_.setClickingTogglesState(true); addAndMakeVisible(autoPreview_);
        import_.setButtonText("Import"); import_.onClick = [this] { importSelected(); }; addAndMakeVisible(import_);
        preview_.setButtonText("Preview"); preview_.onClick = [this] { previewSelected(); }; addAndMakeVisible(preview_);
        stopPreview_.setButtonText("Stop"); stopPreview_.onClick = [this] { stopPreviewNow(); }; addAndMakeVisible(stopPreview_);
        list_.setRowHeight(34); list_.setOutlineThickness(0); list_.setMultipleSelectionEnabled(false); list_.addKeyListener(this); addAndMakeVisible(list_);
        search_.addKeyListener(this);
        hint_.setText("↑/↓ navigate · Home/End jump · Space preview · Enter import · / search", juce::dontSendNotification);
        hint_.setColour(juce::Label::textColourId, juce::Colour(0xff8d95a5)); addAndMakeVisible(hint_);
        previewStatus_.setText("Preview ready", juce::dontSendNotification);
        previewStatus_.setColour(juce::Label::textColourId, juce::Colour(0xff6fca9b)); addAndMakeVisible(previewStatus_);
        refreshRoots(); refresh();
    }

    ~SampleBrowserComponent() override {
        previewGeneration_.fetch_add(1, std::memory_order_relaxed);
        previewPool_.removeAllJobs(true, 5000);
        if (stopPreviewFn_) stopPreviewFn_();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff11141a)); g.setColour(juce::Colour(0xff2a2f38));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 7.0f, 1.0f);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(8); title_.setBounds(r.removeFromTop(24)); search_.setBounds(r.removeFromTop(30));
        auto roots = r.removeFromTop(30); root_.setBounds(roots.removeFromLeft(std::max(0, roots.getWidth()-82)).reduced(2)); addRoot_.setBounds(roots.reduced(2));
        auto actions = r.removeFromTop(30); import_.setBounds(actions.removeFromLeft(48).reduced(2)); preview_.setBounds(actions.removeFromLeft(56).reduced(2)); stopPreview_.setBounds(actions.removeFromLeft(44).reduced(2)); favorite_.setBounds(actions.removeFromLeft(48).reduced(2)); favoritesOnly_.setBounds(actions.removeFromLeft(68).reduced(2)); recent_.setBounds(actions.removeFromLeft(52).reduced(2)); autoPreview_.setBounds(actions.removeFromLeft(48).reduced(2));
        previewStatus_.setBounds(r.removeFromBottom(20)); hint_.setBounds(r.removeFromBottom(20)); list_.setBounds(r.reduced(2));
    }

private:
    int getNumRows() override { return static_cast<int>(visible_.size()); }
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override {
        if (row < 0 || row >= static_cast<int>(visible_.size())) return;
        if (selected) g.fillAll(juce::Colour(0xff263249)); const auto& p = visible_[static_cast<std::size_t>(row)];
        const bool favorite = std::find(settings_.favoriteSamples.begin(), settings_.favoriteSamples.end(), p) != settings_.favoriteSamples.end();
        g.setColour(juce::Colour(0xfff0f3f8)); g.drawText((favorite ? juce::String::fromUTF8("★ ") : juce::String()) + juce::String(p.filename().string()), 8, 2, width-16, 16, juce::Justification::centredLeft, true);
        g.setColour(juce::Colour(0xff7f8796)); g.setFont(11.0f); g.drawText(juce::String(p.parent_path().string()), 8, 18, width-16, std::max(12,height-18), juce::Justification::centredLeft, true);
    }
    void selectedRowsChanged(int) override { updateActions(); if(autoPreview_.getToggleState()&&!selectedPath().empty())previewSelected(); }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override {
        if (row >= 0 && row < static_cast<int>(visible_.size())) importPath(visible_[static_cast<std::size_t>(row)]);
    }
    juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override {
        if (selectedRows.size() != 1) return {};
        const int row = selectedRows[0]; if (row < 0 || row >= static_cast<int>(visible_.size())) return {};
        const auto& p = visible_[static_cast<std::size_t>(row)];
        return juce::String("flowdaw-sample:") + juce::String(p.string());
    }
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override {
        if (key == juce::KeyPress::spaceKey) { previewSelected(); return true; }
        if (key == juce::KeyPress::returnKey) { importSelected(); return true; }
        if (key == juce::KeyPress::escapeKey) { stopPreviewNow(); return true; }
        if (key.getKeyCode() == juce::KeyPress::upKey) { selectRelative(-1); return true; }
        if (key.getKeyCode() == juce::KeyPress::downKey) { selectRelative(1); return true; }
        if (key.getKeyCode() == juce::KeyPress::homeKey) { selectAbsolute(0); return true; }
        if (key.getKeyCode() == juce::KeyPress::endKey) { selectAbsolute(static_cast<int>(visible_.size())-1); return true; }
        if (key.getTextCharacter() == '/') { search_.grabKeyboardFocus(); search_.selectAll(); return true; }
        return false;
    }
    static std::string lower(std::string s) { for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }
    bool matchesSearch(const std::filesystem::path& p) const {
        auto q = lower(search_.getText().trim().toStdString()); if (q.empty()) return true;
        return lower(p.filename().string()+" "+p.parent_path().string()).find(q) != std::string::npos;
    }
    void notifySettingsChanged() { if (settingsChanged_) settingsChanged_(); }
    void refreshRoots() {
        const int previous = root_.getSelectedId(); root_.clear(juce::dontSendNotification); int id=1;
        for (const auto& p : settings_.sampleRoots) root_.addItem(juce::String(p.string()), id++);
        if (!settings_.sampleRoots.empty()) root_.setSelectedId(previous>0 && previous<=static_cast<int>(settings_.sampleRoots.size()) ? previous : 1, juce::dontSendNotification);
    }
    void refresh() {
        const auto previousPath = selectedPath();
        visible_.clear();
        if(showRecent_){
            for(const auto&p:settings_.recentSamples){std::error_code ec;if(std::filesystem::is_regular_file(p,ec)&&lower(p.extension().string())==".wav"&&matchesSearch(p))visible_.push_back(p);}
        }else if(showFavorites_){
            for(const auto&p:settings_.favoriteSamples){std::error_code ec;if(std::filesystem::is_regular_file(p,ec)&&lower(p.extension().string())==".wav"&&matchesSearch(p))visible_.push_back(p);}
            std::sort(visible_.begin(), visible_.end(), [](const auto& a,const auto& b){return lower(a.filename().string()) < lower(b.filename().string());});
        }else{
            const int idx = root_.getSelectedId()-1;
            if (idx >= 0 && idx < static_cast<int>(settings_.sampleRoots.size())) {
                const auto base = settings_.sampleRoots[static_cast<std::size_t>(idx)]; std::error_code ec; std::size_t scanned=0;
                if (std::filesystem::exists(base, ec)) for (std::filesystem::recursive_directory_iterator it(base, std::filesystem::directory_options::skip_permission_denied, ec), end; it!=end && scanned<5000; it.increment(ec)) {
                    if (ec) { ec.clear(); continue; } if (!it->is_regular_file(ec)) continue; ++scanned;
                    if (lower(it->path().extension().string()) != ".wav") continue; if (matchesSearch(it->path())) visible_.push_back(it->path());
                }
                std::sort(visible_.begin(), visible_.end(), [](const auto& a,const auto& b){return lower(a.filename().string()) < lower(b.filename().string());});
            }
        }
        list_.updateContent(); list_.repaint();
        if (!visible_.empty()) {
            int row=0;
            if (!previousPath.empty()) {
                auto it=std::find(visible_.begin(),visible_.end(),previousPath);
                if(it!=visible_.end())row=static_cast<int>(std::distance(visible_.begin(),it));
            }
            list_.selectRow(row);
        } else list_.deselectAllRows();
        updateActions();
    }
    void addRootPath(const std::filesystem::path& p) {
        if (p.empty() || !std::filesystem::is_directory(p)) return;
        if (std::find(settings_.sampleRoots.begin(), settings_.sampleRoots.end(), p) == settings_.sampleRoots.end()) {
            settings_.sampleRoots.push_back(p); if (settings_.sampleRoots.size()>64) settings_.sampleRoots.erase(settings_.sampleRoots.begin()); notifySettingsChanged();
        }
        showRecent_=false; showFavorites_=false; recent_.setToggleState(false, juce::dontSendNotification); favoritesOnly_.setToggleState(false,juce::dontSendNotification); refreshRoots(); for (std::size_t i=0;i<settings_.sampleRoots.size();++i) if (settings_.sampleRoots[i]==p) { root_.setSelectedId(static_cast<int>(i)+1, juce::dontSendNotification); break; } refresh();
    }
    void chooseRoot() {
        chooser_ = std::make_unique<juce::FileChooser>("Add sample folder", juce::File::getSpecialLocation(juce::File::userMusicDirectory));
        chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories, [this](const juce::FileChooser& fc){ auto f=fc.getResult(); if(f.isDirectory()) addRootPath(std::filesystem::path(f.getFullPathName().toStdString())); chooser_.reset(); });
    }
    std::filesystem::path selectedPath() const { const int row=list_.getSelectedRow(); return row>=0 && row<static_cast<int>(visible_.size()) ? visible_[static_cast<std::size_t>(row)] : std::filesystem::path{}; }
    void selectAbsolute(int row) {
        if(visible_.empty())return;
        row=std::clamp(row,0,static_cast<int>(visible_.size())-1);
        list_.selectRow(row);list_.scrollToEnsureRowIsOnscreen(row);updateActions();
    }
    void selectRelative(int delta) {
        if (visible_.empty()) return;
        const int current=list_.getSelectedRow();
        selectAbsolute((current<0?0:current)+delta);
    }
    void updateActions() {
        const bool selected=!selectedPath().empty();
        import_.setEnabled(selected); favorite_.setEnabled(selected); preview_.setEnabled(selected);
    }
    void toggleFavoriteSelected() {
        auto p=selectedPath(); if(p.empty()) return; auto& favorites=settings_.favoriteSamples;
        auto it=std::find(favorites.begin(),favorites.end(),p); if(it==favorites.end()){favorites.push_back(p);if(favorites.size()>256)favorites.erase(favorites.begin());}else favorites.erase(it);
        notifySettingsChanged(); list_.repaint();
    }
    void previewSelected() {
        const auto path=selectedPath(); if(path.empty()||!previewFn_)return;
        const auto generation=previewGeneration_.fetch_add(1,std::memory_order_relaxed)+1;
        previewStatus_.setText("Loading "+juce::String(path.filename().string())+"…",juce::dontSendNotification);
        previewPool_.removeAllJobs(false,0);
        auto safe=juce::Component::SafePointer<SampleBrowserComponent>(this);
        previewPool_.addJob([safe,path,generation]{
            try{
                auto audio=std::make_shared<AudioBuffer>(WavFile::read(path));
                juce::MessageManager::callAsync([safe,path,generation,audio]{
                    if(auto*self=safe.getComponent()){
                        if(self->previewGeneration_.load(std::memory_order_relaxed)!=generation)return;
                        if(audio&&audio->frames()>0&&self->previewFn_)self->previewFn_(audio);
                        const double seconds=audio&&audio->sampleRate>0?static_cast<double>(audio->frames())/audio->sampleRate:0.0;
                        self->previewStatus_.setText("Preview: "+juce::String(path.filename().string())+" • "+juce::String(seconds,1)+"s • "+juce::String(audio?audio->sampleRate:0)+" Hz",juce::dontSendNotification);
                    }
                });
            }catch(const std::exception&e){
                const juce::String error=e.what();
                juce::MessageManager::callAsync([safe,generation,error]{
                    if(auto*self=safe.getComponent()){
                        if(self->previewGeneration_.load(std::memory_order_relaxed)!=generation)return;
                        self->previewStatus_.setText("Preview failed: "+error,juce::dontSendNotification);
                    }
                });
            }
        });
    }
    void stopPreviewNow() {
        previewGeneration_.fetch_add(1,std::memory_order_relaxed);
        if(stopPreviewFn_)stopPreviewFn_();
        previewStatus_.setText("Preview stopped",juce::dontSendNotification);
    }
    void importPath(const std::filesystem::path& p) {
        if(p.empty() || !std::filesystem::exists(p) || !importFn_) return;
        importFn_(p); auto& recent=settings_.recentSamples; recent.erase(std::remove(recent.begin(),recent.end(),p),recent.end()); recent.insert(recent.begin(),p); if(recent.size()>64)recent.resize(64); notifySettingsChanged(); if(showRecent_)refresh();
    }
    void importSelected() { importPath(selectedPath()); }

    AppSettings& settings_; ImportFn importFn_; PreviewFn previewFn_; StopPreviewFn stopPreviewFn_; SettingsChangedFn settingsChanged_;
    std::vector<std::filesystem::path> visible_; std::unique_ptr<juce::FileChooser> chooser_; bool showRecent_=false,showFavorites_=false;
    std::atomic<std::uint64_t> previewGeneration_{0}; juce::ThreadPool previewPool_;
    juce::Label title_, hint_, previewStatus_; juce::TextEditor search_; juce::ComboBox root_;
    juce::TextButton addRoot_, favorite_, recent_, favoritesOnly_, import_, preview_, stopPreview_; juce::ToggleButton autoPreview_; juce::ListBox list_;
};
}
