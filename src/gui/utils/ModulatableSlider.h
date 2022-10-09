#pragma once

#include "HostContextProvider.h"

class ModulatableSlider : public Slider,
                          private Timer
{
public:
    ModulatableSlider (const chowdsp::FloatParameter& param, const HostContextProvider& hostContextProvider);

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;

private:
    void timerCallback() override;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float modSliderPos);
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float modSliderPos);

    const chowdsp::FloatParameter& param;
    const HostContextProvider& hostContextProvider;

    double modulatedValue = param.getCurrentValue();

    struct KnobAssets
    {
        std::unique_ptr<Drawable> knob = Drawable::createFromImageData (chowdsp_BinaryData::knob_svg, chowdsp_BinaryData::knob_svgSize);
        std::unique_ptr<Drawable> pointer = Drawable::createFromImageData (chowdsp_BinaryData::pointer_svg, chowdsp_BinaryData::pointer_svgSize);
    };
    SharedResourcePointer<KnobAssets> sharedAssets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulatableSlider)
};
