#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace flowdaw::juceui {

// Pure menu presentation: IDs are routed by the host's existing command dispatcher.
// This file does not own project state, audio callbacks or plugin instances.
enum class CompactMenuGroup { File, Edit, View, Tools, Help };

inline juce::PopupMenu makeCompactMenu(CompactMenuGroup group, bool mixerVisible) {
    juce::PopupMenu menu;
    switch (group) {
    case CompactMenuGroup::File:
        menu.addItem(1, "New / Template    Ctrl/Cmd+N");
        menu.addItem(2, "Open Project    Ctrl/Cmd+O");
        menu.addItem(4, "Import WAV    Ctrl/Cmd+I");
        menu.addSeparator();
        menu.addItem(3, "Save Project    Ctrl/Cmd+S");
        menu.addItem(8, "Export Mix / Stems...");
        break;
    case CompactMenuGroup::Edit:
        menu.addItem(6, "Undo    Ctrl/Cmd+Z");
        menu.addItem(7, "Redo    Ctrl/Cmd+Shift+Z");
        break;
    case CompactMenuGroup::View:
        menu.addItem(11, "Arrangement    Ctrl/Cmd+1");
        menu.addItem(12, "Piano Roll    Ctrl/Cmd+2");
        menu.addItem(13, "Sequencer    Ctrl/Cmd+3");
        menu.addItem(14, "Automation    Ctrl/Cmd+4");
        menu.addItem(15, "Sampler    Ctrl/Cmd+5");
        menu.addSeparator();
        menu.addItem(16, mixerVisible ? "Hide Mixer" : "Show Mixer");
        menu.addItem(17, "Audio I/O Settings");
        menu.addItem(18, "Plugin Maintenance");
        break;
    case CompactMenuGroup::Tools:
        menu.addItem(19, "Scan VST3 / AU Plugins");
        menu.addItem(9, "Commands    Ctrl/Cmd+K");
        break;
    case CompactMenuGroup::Help:
        menu.addItem(20, "Welcome / Quick Start");
        menu.addItem(9, "Keyboard Commands    Ctrl/Cmd+K");
        break;
    }
    return menu;
}
} // namespace flowdaw::juceui
