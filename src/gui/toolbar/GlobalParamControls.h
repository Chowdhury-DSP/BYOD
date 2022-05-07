#pragma once

#include "TextSlider.h"

class GlobalParamControls : public Component
{
public:
    GlobalParamControls (AudioProcessorValueTreeState& vts, chowdsp::VariableOversampling<float>& oversampling);

    void resized() override;

private:
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;

    struct SliderWithAttachment : public TextSlider
    {
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct BoxWithAttachment : public ComboBox
    {
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    OwnedArray<ComboBox> boxes;
    OwnedArray<Slider> sliders;

    using OSMenu = chowdsp::OversamplingMenu<chowdsp::VariableOversampling<float>>;
    OSMenu osMenu;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlobalParamControls)
};
