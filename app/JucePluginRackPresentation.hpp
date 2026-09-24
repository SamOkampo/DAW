#pragma once

#include "JuceTheme.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

namespace flowdaw::juceui {

// Presentation-only helper for the existing FLOWDAW insert rack controls.
// It owns no project/plugin state and performs no audio, filesystem, scanning
// or plugin-instantiation work. Existing callbacks remain the authority.
class PluginRackPresentation final {
public:
    static void style(juce::ComboBox& target,
                      juce::ComboBox& insert,
                      juce::Slider& wet,
                      juce::Slider& parameter,
                      juce::Label& parameterLabel,
                      juce::TextButton& moveUp,
                      juce::TextButton& moveDown,
                      juce::TextButton& enabled,
                      juce::TextButton& bypass,
                      juce::TextButton& remove,
                      juce::TextButton& editor,
                      juce::LookAndFeel& lookAndFeel)
    {
        target.setTextWhenNothingSelected("INSERT RACK  •  choose channel");
        insert.setTextWhenNothingSelected("SIGNAL CHAIN  •  empty");
        wet.setTextValueSuffix(" wet");
        parameterLabel.setText("INSERT PARAMETER", juce::dontSendNotification);
        parameterLabel.setFont(juce::Font(10.5f, juce::Font::bold));
        parameterLabel.setColour(juce::Label::textColourId, FlowTheme::textMuted());

        moveUp.setButtonText("↑ ORDER");
        moveDown.setButtonText("↓ ORDER");
        enabled.setButtonText("ACTIVE");
        bypass.setButtonText("BYPASS");
        remove.setButtonText("REMOVE");
        editor.setButtonText("OPEN INSERT");

        for (auto* choice : { &target, &insert }) choice->setLookAndFeel(&lookAndFeel);
        for (auto* slider : { &wet, &parameter }) slider->setLookAndFeel(&lookAndFeel);
        for (auto* button : { &moveUp, &moveDown, &enabled, &bypass, &remove, &editor })
            button->setLookAndFeel(&lookAndFeel);

        enabled.setColour(juce::TextButton::buttonOnColourId, FlowTheme::aqua());
        bypass.setColour(juce::TextButton::buttonOnColourId, FlowTheme::accentHot());
        remove.setColour(juce::TextButton::buttonOnColourId, FlowTheme::accentHot());
    }

    static void clear(juce::ComboBox& target,
                      juce::ComboBox& insert,
                      juce::Slider& wet,
                      juce::Slider& parameter,
                      juce::TextButton& moveUp,
                      juce::TextButton& moveDown,
                      juce::TextButton& enabled,
                      juce::TextButton& bypass,
                      juce::TextButton& remove,
                      juce::TextButton& editor)
    {
        target.setLookAndFeel(nullptr);
        insert.setLookAndFeel(nullptr);
        wet.setLookAndFeel(nullptr);
        parameter.setLookAndFeel(nullptr);
        for (auto* button : { &moveUp, &moveDown, &enabled, &bypass, &remove, &editor })
            button->setLookAndFeel(nullptr);
    }
};

} // namespace flowdaw::juceui
