#pragma once

#include "LookAndFeels.h"

/* Slider that shows only a text bubble */
class TextSlider : public Slider
{
public:
    TextSlider()
    {
        setLookAndFeel (lnfAllocator->getLookAndFeel<BottomBarLNF>());
        setMouseDragSensitivity (500);
    }

    ~TextSlider() override
    {
        setLookAndFeel (nullptr);
    }

    void mouseUp (const MouseEvent& e) override
    {
        Slider::mouseUp (e);

        multiClicking = e.getNumberOfClicks() > 1;
        bool dontShowLabel = e.mouseWasDraggedSinceMouseDown() || e.mods.isAnyModifierKeyDown() || e.mods.isPopupMenu() || multiClicking;
        if (! dontShowLabel)
        {
            Timer::callAfterDelay (270,
                                   [=]
                                   {
                                       if (multiClicking)
                                       {
                                           multiClicking = false;
                                           return;
                                       }

                                       showTextBox();
                                   });
        }
    }

private:
    bool multiClicking = false;
    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextSlider)
};
