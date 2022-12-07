#pragma once

#include <pch.h>

class ModulatableSlider : public Slider,
                          private Timer
{
public:
    ModulatableSlider (const chowdsp::FloatParameter& param, const chowdsp::HostContextProvider& hostContextProvider);

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;
    auto& getParameter() const { return param; }

    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> attachment;

protected:
    void timerCallback() override;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float modSliderPos);
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float modSliderPos);

    const chowdsp::FloatParameter& param;
    const chowdsp::HostContextProvider& hostContextProvider;

    double modulatedValue = param.getCurrentValue();

    struct KnobAssets
    {
        std::unique_ptr<Drawable> knob = Drawable::createFromImageData (chowdsp_BinaryData::knob_svg, chowdsp_BinaryData::knob_svgSize);
        std::unique_ptr<Drawable> pointer = Drawable::createFromImageData (chowdsp_BinaryData::pointer_svg, chowdsp_BinaryData::pointer_svgSize);
    };
    SharedResourcePointer<KnobAssets> sharedAssets;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulatableSlider)
};
