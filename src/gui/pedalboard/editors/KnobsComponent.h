#pragma once

#include "gui/utils/ModulatableSlider.h"
#include "processors/BaseProcessor.h"

class KnobsComponent : public Component
{
public:
    KnobsComponent (BaseProcessor& baseProc,
                    AudioProcessorValueTreeState& vts,
                    const Colour& contrastColour,
                    const Colour& accentColour,
                    const HostContextProvider& hostContextProvider);

    void paint (Graphics& g) override;
    void resized() override;

    void toggleParamsEnabled (const std::vector<String>& paramIDs, bool shouldEnable);

    void setColours (const Colour& contrastColour, const Colour& accentColour);

private:
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

    struct SliderWithAttachment : public ModulatableSlider
    {
        SliderWithAttachment (const chowdsp::FloatParameter& param, const HostContextProvider& hcp) : ModulatableSlider (param, hcp) {}
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
    std::unordered_set<String> sliderNames;
    std::unordered_set<String> boxNames;

    Colour contrastColour;
    Colour accentColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobsComponent)
};
