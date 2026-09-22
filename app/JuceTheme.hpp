#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>

namespace flowdaw::juceui {

struct FlowTheme final {
    static juce::Colour canvasTop(){return juce::Colour(0xff0b0914);}
    static juce::Colour canvasBottom(){return juce::Colour(0xff070b10);}
    static juce::Colour surface(){return juce::Colour(0xff121421);}
    static juce::Colour surfaceRaised(){return juce::Colour(0xff1a1b2a);}
    static juce::Colour surfaceHover(){return juce::Colour(0xff242438);}
    static juce::Colour borderSubtle(){return juce::Colour(0xff303044);}
    static juce::Colour borderStrong(){return juce::Colour(0xff4a4964);}
    static juce::Colour textPrimary(){return juce::Colour(0xfff5f2fb);}
    static juce::Colour textSecondary(){return juce::Colour(0xffbbb7c9);}
    static juce::Colour textMuted(){return juce::Colour(0xff817d91);}
    static juce::Colour accent(){return juce::Colour(0xffe45b98);}
    static juce::Colour accentHot(){return juce::Colour(0xffff7ab5);}
    static juce::Colour accentDeep(){return juce::Colour(0xff8f4cff);}
    static juce::Colour aqua(){return juce::Colour(0xff52d6c7);}
    static juce::Colour focus(){return juce::Colour(0xff85e8dc);}
    static juce::Colour success(){return juce::Colour(0xff59d69a);}
    static juce::Colour warning(){return juce::Colour(0xffffbe63);}
    static juce::Colour danger(){return juce::Colour(0xffff667d);}
    static juce::Colour meterSafe(){return juce::Colour(0xff52d6c7);}
    static juce::Colour meterHot(){return juce::Colour(0xffffbe63);}
    static juce::Colour meterClip(){return juce::Colour(0xffff667d);}

    static constexpr int space1=4;
    static constexpr int space2=8;
    static constexpr int space3=12;
    static constexpr int space4=16;
    static constexpr int space5=24;
    static constexpr int space6=32;
    static constexpr float radiusSmall=6.0f;
    static constexpr float radiusPanel=14.0f;
};

inline juce::Path makeNotchedControlPath(juce::Rectangle<float> r,float notch=7.0f){
    notch=std::min(notch,r.getHeight()*0.28f);
    juce::Path p;
    p.startNewSubPath(r.getX()+notch,r.getY());
    p.lineTo(r.getRight()-notch*0.65f,r.getY());
    p.lineTo(r.getRight(),r.getY()+notch);
    p.lineTo(r.getRight(),r.getBottom()-notch);
    p.lineTo(r.getRight()-notch,r.getBottom());
    p.lineTo(r.getX()+notch*0.65f,r.getBottom());
    p.lineTo(r.getX(),r.getBottom()-notch);
    p.lineTo(r.getX(),r.getY()+notch);
    p.closeSubPath();
    return p;
}

class ShellLookAndFeel final:public juce::LookAndFeel_V4 {
public:
    ShellLookAndFeel(){
        setColour(juce::TextButton::textColourOffId,FlowTheme::textSecondary());
        setColour(juce::TextButton::textColourOnId,FlowTheme::textPrimary());
        setColour(juce::ComboBox::textColourId,FlowTheme::textPrimary());
        setColour(juce::ComboBox::backgroundColourId,juce::Colours::transparentBlack);
        setColour(juce::ComboBox::outlineColourId,juce::Colours::transparentBlack);
        setColour(juce::ComboBox::arrowColourId,FlowTheme::aqua());
        setColour(juce::PopupMenu::backgroundColourId,FlowTheme::surfaceRaised());
        setColour(juce::PopupMenu::textColourId,FlowTheme::textPrimary());
        setColour(juce::PopupMenu::highlightedBackgroundColourId,FlowTheme::accentDeep().withAlpha(0.72f));
        setColour(juce::PopupMenu::highlightedTextColourId,FlowTheme::textPrimary());
    }

    void drawButtonBackground(juce::Graphics&g,juce::Button&button,const juce::Colour&backgroundColour,bool isMouseOver,bool isButtonDown)override{
        (void)backgroundColour;
        auto r=button.getLocalBounds().toFloat().reduced(0.75f);
        const bool active=button.getToggleState();
        const bool transportPlay=button.getComponentID()=="transport-play";
        auto top=active?(transportPlay?FlowTheme::aqua().darker(0.42f):FlowTheme::accentDeep()):FlowTheme::surfaceRaised();
        auto bottom=active?(transportPlay?FlowTheme::aqua().darker(0.72f):FlowTheme::accent().darker(0.28f)):FlowTheme::surface();
        if(isMouseOver){top=top.interpolatedWith(FlowTheme::aqua(),active?0.14f:0.09f);bottom=bottom.brighter(0.08f);}
        if(isButtonDown){top=top.darker(0.16f);bottom=bottom.darker(0.12f);}
        if(!button.isEnabled()){top=top.withMultipliedAlpha(0.42f);bottom=bottom.withMultipliedAlpha(0.42f);}

        auto shape=makeNotchedControlPath(r);
        juce::ColourGradient fill(top,r.getX(),r.getY(),bottom,r.getRight(),r.getBottom(),false);
        fill.addColour(0.56,active?(transportPlay?FlowTheme::aqua().withAlpha(0.88f):FlowTheme::accent().withAlpha(0.94f)):FlowTheme::surfaceHover().withAlpha(0.88f));
        g.setGradientFill(fill);
        g.fillPath(shape);

        g.setColour((active?(transportPlay?FlowTheme::focus():FlowTheme::accentHot()):FlowTheme::borderSubtle()).withAlpha(button.isEnabled()?0.90f:0.42f));
        g.strokePath(shape,juce::PathStrokeType(active?1.35f:1.0f));

        if(button.hasKeyboardFocus(true)){
            auto focusShape=makeNotchedControlPath(r.reduced(2.0f),5.0f);
            g.setColour(FlowTheme::focus().withAlpha(0.92f));
            g.strokePath(focusShape,juce::PathStrokeType(1.2f));
        }
    }

    void drawComboBox(juce::Graphics&g,int width,int height,bool isButtonDown,int buttonX,int buttonY,int buttonW,int buttonH,juce::ComboBox&box)override{
        (void)buttonX;(void)buttonY;(void)buttonW;(void)buttonH;
        auto r=juce::Rectangle<float>(0.5f,0.5f,static_cast<float>(width)-1.0f,static_cast<float>(height)-1.0f);
        auto shape=makeNotchedControlPath(r,8.0f);
        auto top=isButtonDown?FlowTheme::surfaceHover():FlowTheme::surfaceRaised();
        juce::ColourGradient fill(top,r.getX(),r.getY(),FlowTheme::surface(),r.getRight(),r.getBottom(),false);
        fill.addColour(0.70,FlowTheme::accentDeep().withAlpha(0.16f));
        g.setGradientFill(fill);g.fillPath(shape);
        g.setColour(box.hasKeyboardFocus(true)?FlowTheme::focus():FlowTheme::borderSubtle());
        g.strokePath(shape,juce::PathStrokeType(box.hasKeyboardFocus(true)?1.3f:1.0f));

        const float cx=static_cast<float>(width)-15.0f;
        const float cy=static_cast<float>(height)*0.5f;
        juce::Path arrow;arrow.startNewSubPath(cx-4.0f,cy-2.0f);arrow.lineTo(cx,cy+2.0f);arrow.lineTo(cx+4.0f,cy-2.0f);
        g.setColour(FlowTheme::aqua().withAlpha(box.isEnabled()?0.92f:0.35f));
        g.strokePath(arrow,juce::PathStrokeType(1.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }

    void drawLabel(juce::Graphics&g,juce::Label&label)override{
        const auto id=label.getComponentID();
        if(id=="transport-bpm"){
            auto r=label.getLocalBounds().toFloat().reduced(0.75f);
            auto shape=makeNotchedControlPath(r,7.0f);
            juce::ColourGradient fill(FlowTheme::accentDeep().withAlpha(0.38f),r.getX(),r.getY(),FlowTheme::aqua().darker(0.72f).withAlpha(0.72f),r.getRight(),r.getBottom(),false);
            fill.addColour(0.55,FlowTheme::surfaceRaised().withAlpha(0.96f));
            g.setGradientFill(fill);g.fillPath(shape);
            g.setColour(FlowTheme::aqua().withAlpha(label.isEnabled()?0.82f:0.34f));
            g.strokePath(shape,juce::PathStrokeType(1.1f));
            g.setColour(FlowTheme::textPrimary().withMultipliedAlpha(label.isEnabled()?1.0f:0.45f));
            g.setFont(label.getFont());
            g.drawFittedText(label.getText(),label.getBorderSize().subtractedFrom(label.getLocalBounds()),label.getJustificationType(),1,0.92f);
            return;
        }
        if(id=="project-identity"){
            auto r=label.getLocalBounds().toFloat();
            juce::ColourGradient underline(FlowTheme::accent().withAlpha(0.0f),r.getX(),r.getBottom()-2.0f,FlowTheme::aqua().withAlpha(0.68f),r.getRight(),r.getBottom()-2.0f,false);
            g.setGradientFill(underline);g.fillRect(r.getX(),r.getBottom()-1.5f,r.getWidth(),1.0f);
        }
        juce::LookAndFeel_V4::drawLabel(g,label);
    }
};

} // namespace flowdaw::juceui
