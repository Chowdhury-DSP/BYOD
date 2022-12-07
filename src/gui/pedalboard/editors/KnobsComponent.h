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
                    chowdsp::HostContextProvider& hostContextProvider);

    void paint (Graphics& g) override;
    void resized() override;

    void toggleParamsEnabled (const std::vector<String>& paramIDs, bool shouldEnable);

    void setColours (const Colour& contrastColour, const Colour& accentColour);

private:
    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

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

    Colour contrastColour;
    Colour accentColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobsComponent)
};
