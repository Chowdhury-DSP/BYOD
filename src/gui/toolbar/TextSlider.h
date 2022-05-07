#pragma once

#include "gui/utils/LookAndFeels.h"

/* Slider that shows only a text bubble */
class TextSlider : public Slider
{
public:
    TextSlider()
    {
        setLookAndFeel (lnfAllocator->getLookAndFeel<BottomBarLNF>());
        setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        setMouseDragSensitivity (500);
    }

    ~TextSlider() override
    {
        setLookAndFeel (nullptr);
    }

    void resized() override
    {
        Slider::resized();
        setTextBoxStyle (Slider::TextBoxLeft, false, 0, getHeight());
    }

    void mouseUp (const MouseEvent& e) override
    {
        Slider::mouseUp (e);

        multiClicking = e.getNumberOfClicks() > 1;
        bool dontShowLabel = e.mouseWasDraggedSinceMouseDown() || e.mods.isAnyModifierKeyDown() || e.mods.isPopupMenu() || multiClicking;
        if (! dontShowLabel)
        {
            Timer::callAfterDelay (270,
                                   [safeComp = Component::SafePointer (this)]
                                   {
                                       if (safeComp == nullptr)
                                           return; // this component was deleted while waiting for the timer!

                                       if (safeComp->multiClicking)
                                       {
                                           safeComp->multiClicking = false;
                                           return;
                                       }

                                       safeComp->showTextBox();
                                   });
        }
    }

private:
    bool multiClicking = false;
    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextSlider)
};
