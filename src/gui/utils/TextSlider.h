#pragma once

#include "LookAndFeels.h"

/* Slider that shows only a text bubble */
class TextSlider : public Slider
{
public:
    TextSlider()
    {
        setLookAndFeel (lnfAllocator->getLookAndFeel<BottomBarLNF>());
    }

    ~TextSlider() override
    {
        setLookAndFeel (nullptr);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        isDragging = true;
        Slider::mouseDrag (e);
    }

    void mouseDoubleClick (const MouseEvent& e) override
    {
        hideTextBox (false);
        Slider::mouseDoubleClick (e);
    }

    void mouseUp (const MouseEvent& e) override
    {
        Slider::mouseUp (e);

        if (! isDragging && ! e.mods.isAnyModifierKeyDown())
            showTextBox();

        isDragging = false;
    }

private:
    SharedResourcePointer<chowdsp::LNFAllocator> lnfAllocator;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextSlider)
};
