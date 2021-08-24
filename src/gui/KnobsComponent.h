#pragma once

#include <pch.h>

class KnobsComponent : public Component
{
public:
    KnobsComponent (AudioProcessorValueTreeState& vts, const Colour& contrastColour, const Colour& accentColour, std::function<void()> paramLambda = {});

    void paint (Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

    struct SliderWithAttachment
    {
        Slider slider;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct BoxWithAttachment
    {
        ComboBox box;
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    struct ButtonWithAttachment
    {
        TextButton button;
        std::unique_ptr<ButtonAttachment> attachment;
    };

    OwnedArray<SliderWithAttachment> sliders;
    OwnedArray<BoxWithAttachment> boxes;
    OwnedArray<ButtonWithAttachment> buttons;

    const Colour& contrastColour;
    const Colour& accentColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobsComponent)
};
