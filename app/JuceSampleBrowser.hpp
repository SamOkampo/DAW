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

namespace flowdaw::juceui {
class SampleBrowserComponent final : public juce::Component, private juce::ListBoxModel {
public:
    using ImportFn = std::function<void(const std::filesystem::path&)>;
    using SettingsChangedFn = std::function<void()>;

    SampleBrowserComponent(AppSettings& settings, ImportFn importFn, SettingsChangedFn settingsChanged)
        : settings_(settings), importFn_(std::move(importFn)), settingsChanged_(std::move(settingsChanged)), list_("Samples", this) {
        title_.setText("SAMPLE BROWSER", juce::dontSendNotification);
        title_.setFont(juce::Font(14.0f, juce::Font::bold)); addAndMakeVisible(title_);
        search_.setTextToShowWhenEmpty("Search WAVs...", juce::Colour(0xff7f8796));
        search_.onTextChange = [this] { refresh(); }; addAndMakeVisible(search_);
        root_.setTextWhenNothingSelected("No sample folder"); root_.onChange = [this] { refresh(); }; addAndMakeVisible(root_);
        addRoot_.setButtonText("+ Folder"); addRoot_.onClick = [this] { chooseRoot(); }; addAndMakeVisible(addRoot_);
        favorite_.setButtonText("Favorite"); favorite_.onClick = [this] { toggleFavoriteSelected(); }; addAndMakeVisible(favorite_);
        import_.setButtonText("Import"); import_.onClick = [this] { importSelected(); }; addAndMakeVisible(import_);
        list_.setRowHeight(34); list_.setOutlineThickness(0); addAndMakeVisible(list_);
        hint_.setText("Double-click to import · drag WAVs to the DAW", juce::dontSendNotification);
        hint_.setColour(juce::Label::textColourId, juce::Colour(0xff8d95a5)); addAndMakeVisible(hint_);
        refreshRoots(); refresh();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff11141a)); g.setColour(juce::Colour(0xff2a2f38));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 7.0f, 1.0f);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(8); title_.setBounds(r.removeFromTop(24)); search_.setBounds(r.removeFromTop(30));
        auto roots = r.removeFromTop(30); root_.setBounds(roots.removeFromLeft(std::max(0, roots.getWidth()-82)).reduced(2)); addRoot_.setBounds(roots.reduced(2));
        auto actions = r.removeFromTop(30); import_.setBounds(actions.removeFromLeft(72).reduced(2)); favorite_.setBounds(actions.removeFromLeft(82).reduced(2));
        hint_.setBounds(r.removeFromBottom(22)); list_.setBounds(r.reduced(2));
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
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override {
        if (row >= 0 && row < static_cast<int>(visible_.size())) importPath(visible_[static_cast<std::size_t>(row)]);
    }
    juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override {
        if (selectedRows.size() != 1) return {};
        const int row = selectedRows[0]; if (row < 0 || row >= static_cast<int>(visible_.size())) return {};
        const auto& p = visible_[static_cast<std::size_t>(row)];
        return juce::String("flowdaw-sample:") + juce::String(p.string());
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
        visible_.clear(); const int idx = root_.getSelectedId()-1;
        if (idx >= 0 && idx < static_cast<int>(settings_.sampleRoots.size())) {
            const auto base = settings_.sampleRoots[static_cast<std::size_t>(idx)]; std::error_code ec; std::size_t scanned=0;
            if (std::filesystem::exists(base, ec)) for (std::filesystem::recursive_directory_iterator it(base, std::filesystem::directory_options::skip_permission_denied, ec), end; it!=end && scanned<5000; it.increment(ec)) {
                if (ec) { ec.clear(); continue; } if (!it->is_regular_file(ec)) continue; ++scanned;
                if (lower(it->path().extension().string()) != ".wav") continue; if (matchesSearch(it->path())) visible_.push_back(it->path());
            }
            std::sort(visible_.begin(), visible_.end(), [](const auto& a,const auto& b){return lower(a.filename().string()) < lower(b.filename().string());});
        }
        list_.updateContent(); list_.repaint(); import_.setEnabled(!visible_.empty()); favorite_.setEnabled(!visible_.empty());
    }
    void addRootPath(const std::filesystem::path& p) {
        if (p.empty() || !std::filesystem::is_directory(p)) return;
        if (std::find(settings_.sampleRoots.begin(), settings_.sampleRoots.end(), p) == settings_.sampleRoots.end()) {
            settings_.sampleRoots.push_back(p); if (settings_.sampleRoots.size()>64) settings_.sampleRoots.erase(settings_.sampleRoots.begin()); notifySettingsChanged();
        }
        refreshRoots(); for (std::size_t i=0;i<settings_.sampleRoots.size();++i) if (settings_.sampleRoots[i]==p) { root_.setSelectedId(static_cast<int>(i)+1, juce::dontSendNotification); break; } refresh();
    }
    void chooseRoot() {
        chooser_ = std::make_unique<juce::FileChooser>("Add sample folder", juce::File::getSpecialLocation(juce::File::userMusicDirectory));
        chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories, [this](const juce::FileChooser& fc){ auto f=fc.getResult(); if(f.isDirectory()) addRootPath(std::filesystem::path(f.getFullPathName().toStdString())); chooser_.reset(); });
    }
    std::filesystem::path selectedPath() const { const int row=list_.getSelectedRow(); return row>=0 && row<static_cast<int>(visible_.size()) ? visible_[static_cast<std::size_t>(row)] : std::filesystem::path{}; }
    void toggleFavoriteSelected() {
        auto p=selectedPath(); if(p.empty()) return; auto& favorites=settings_.favoriteSamples;
        auto it=std::find(favorites.begin(),favorites.end(),p); if(it==favorites.end()){favorites.push_back(p);if(favorites.size()>256)favorites.erase(favorites.begin());}else favorites.erase(it);
        notifySettingsChanged(); list_.repaint();
    }
    void importPath(const std::filesystem::path& p) {
        if(p.empty() || !std::filesystem::exists(p) || !importFn_) return;
        importFn_(p); auto& recent=settings_.recentSamples; recent.erase(std::remove(recent.begin(),recent.end(),p),recent.end()); recent.insert(recent.begin(),p); if(recent.size()>64)recent.resize(64); notifySettingsChanged();
    }
    void importSelected() { importPath(selectedPath()); }

    AppSettings& settings_; ImportFn importFn_; SettingsChangedFn settingsChanged_; std::vector<std::filesystem::path> visible_; std::unique_ptr<juce::FileChooser> chooser_;
    juce::Label title_, hint_; juce::TextEditor search_; juce::ComboBox root_; juce::TextButton addRoot_, favorite_, import_; juce::ListBox list_;
};
}
