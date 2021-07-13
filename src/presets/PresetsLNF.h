#pragma once

#include <pch.h>

class PresetsLNF : public chowdsp::ChowLNF
{
public:
    PresetsLNF()
    {
        setColour (PopupMenu::backgroundColourId, Colour (0xFF31323A));
        setColour (PopupMenu::highlightedBackgroundColourId, Colour (0x7FEAA92C));
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
    }

    void drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box) override
    {
        auto cornerSize = 5.0f;
        Rectangle<int> boxBounds (0, 0, width, height);

        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
    }

    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (3, 1, box.getWidth(), box.getHeight() - 2);
        label.setFont (getComboBoxFont (box).boldened());
    }

    void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool /*isTicked*/, const bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* const textColourToUse) override
    {
        LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, isHighlighted, false /*isTicked*/, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
    }

    void drawPopupMenuBackground (Graphics& g, int width, int height) override
    {
        g.fillAll (findColour (PopupMenu::backgroundColourId));
        ignoreUnused (width, height);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsLNF)
};
