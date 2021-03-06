#pragma once

#include "processors/BaseProcessor.h"

class KnobsComponent : public Component
{
public:
    KnobsComponent (BaseProcessor& baseProc, AudioProcessorValueTreeState& vts, const Colour& contrastColour, const Colour& accentColour);

    void paint (Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

    struct SliderWithAttachment : public Slider
    {
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct BoxWithAttachment : public ComboBox
    {
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    struct ButtonWithAttachment : public TextButton
    {
        std::unique_ptr<ButtonAttachment> attachment;
    };

    OwnedArray<Slider> sliders;
    OwnedArray<ComboBox> boxes;
    OwnedArray<TextButton> buttons;
    OwnedArray<Component> customComponents;

    const Colour& contrastColour;
    const Colour& accentColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobsComponent)
};
