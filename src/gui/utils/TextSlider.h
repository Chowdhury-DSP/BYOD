#pragma once

#include "HostContextProvider.h"
#include "LookAndFeels.h"

/* Slider that shows only a text bubble */
class TextSlider : public Slider
{
public:
    TextSlider (const chowdsp::FloatParameter* param, const HostContextProvider& hostContextProvider);
    ~TextSlider() override;

    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

private:
    bool multiClicking = false;
    chowdsp::SharedLNFAllocator lnfAllocator;

    const chowdsp::FloatParameter* parameter;
    const HostContextProvider& hostContextProvider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextSlider)
};
