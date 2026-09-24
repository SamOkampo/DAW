#pragma once

#include "JuceTheme.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

namespace flowdaw::juceui {

// Presentation-only helper for the existing FLOWDAW Track routing/send controls.
// It owns no project state and performs no audio, filesystem or plugin work.
class MixerRoutingPresentation final {
public:
    static void style(juce::Label& section,
                      juce::ComboBox& output,
                      juce::ComboBox& sendBus,
                      juce::Slider& sendGain,
                      juce::ToggleButton& prePost,
                      juce::TextButton& setSend,
                      juce::TextButton& removeSend,
                      juce::LookAndFeel& lookAndFeel)
    {
        section.setText("SIGNAL PATH  /  ROUTING + SEND", juce::dontSendNotification);
        section.setFont(juce::Font(11.0f, juce::Font::bold));
        section.setColour(juce::Label::textColourId, FlowTheme::aqua().withAlpha(0.88f));

        output.setTextWhenNothingSelected("OUTPUT  •  Master / Bus");
        sendBus.setTextWhenNothingSelected("SEND  •  choose bus");
        sendGain.setTextValueSuffix(" send");
        prePost.setButtonText("PRE");
        setSend.setButtonText("APPLY SEND");
        removeSend.setButtonText("CLEAR");

        output.setLookAndFeel(&lookAndFeel);
        sendBus.setLookAndFeel(&lookAndFeel);
        sendGain.setLookAndFeel(&lookAndFeel);
        prePost.setLookAndFeel(&lookAndFeel);
        setSend.setLookAndFeel(&lookAndFeel);
        removeSend.setLookAndFeel(&lookAndFeel);

        prePost.setColour(juce::ToggleButton::textColourId, FlowTheme::textSecondary());
        setSend.setColour(juce::TextButton::buttonOnColourId, FlowTheme::aqua());
        removeSend.setColour(juce::TextButton::buttonOnColourId, FlowTheme::accentHot());
    }

    static void clear(juce::ComboBox& output,
                      juce::ComboBox& sendBus,
                      juce::Slider& sendGain,
                      juce::ToggleButton& prePost,
                      juce::TextButton& setSend,
                      juce::TextButton& removeSend)
    {
        output.setLookAndFeel(nullptr);
        sendBus.setLookAndFeel(nullptr);
        sendGain.setLookAndFeel(nullptr);
        prePost.setLookAndFeel(nullptr);
        setSend.setLookAndFeel(nullptr);
        removeSend.setLookAndFeel(nullptr);
    }

    static void layout(juce::Rectangle<int> row,
                       juce::Label& section,
                       juce::ComboBox& output,
                       juce::ComboBox& sendBus,
                       juce::Slider& sendGain,
                       juce::ToggleButton& prePost,
                       juce::TextButton& setSend,
                       juce::TextButton& removeSend)
    {
        row.removeFromLeft(8);
        section.setBounds(row.removeFromLeft(174).reduced(4));
        output.setBounds(row.removeFromLeft(218).reduced(4));
        row.removeFromLeft(8);
        sendBus.setBounds(row.removeFromLeft(190).reduced(4));
        sendGain.setBounds(row.removeFromLeft(178).reduced(4));
        prePost.setBounds(row.removeFromLeft(62).reduced(4));
        setSend.setBounds(row.removeFromLeft(106).reduced(4));
        removeSend.setBounds(row.removeFromLeft(76).reduced(4));
    }
};

} // namespace flowdaw::juceui
