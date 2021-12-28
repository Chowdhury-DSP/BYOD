#pragma once

#include <pch.h>

class ByodLNF : public chowdsp::ChowLNF
{
public:
    ByodLNF() = default;
    ~ByodLNF() override = default;

    void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool /*isTicked*/, const bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* const textColourToUse) override;
    Component* getParentComponentForMenuOptions (const PopupMenu::Options& options) override;
    void drawPopupMenuBackground (Graphics& g, int width, int height) override;

    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    Font getTextButtonFont (TextButton&, int buttonHeight) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ByodLNF)
};

class ProcessorLNF : public ByodLNF
{
public:
    ProcessorLNF();
    ~ProcessorLNF() override = default;

    void positionComboBoxText (ComboBox& box, Label& label) override;
    void drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box) override;
    Font getComboBoxFont (ComboBox& box) override;

    Slider::SliderLayout getSliderLayout (Slider& slider) override;
    Label* createSliderTextBox (Slider& slider) override;

    void drawLabel (Graphics& g, Label& label) override;
    Font getLabelFont (Label& label) override;

private:
    static constexpr float labelWidthPct = 0.65f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorLNF)
};

/** Look and feel for the bottom bar section */
class BottomBarLNF : public chowdsp::ChowLNF
{
public:
    BottomBarLNF() = default;
    ~BottomBarLNF() override = default;

protected:
    static int getNameWidth (int height, const String& text);
    void drawRotarySlider (Graphics& g, int, int, int, int height, float, const float, const float, Slider& slider) override;
    Slider::SliderLayout getSliderLayout (Slider& slider) override;
    Label* createSliderTextBox (Slider& slider) override;

private:
    static constexpr float heightFrac = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomBarLNF)
};
