#pragma once

#include "gui/utils/TextSlider.h"

class GlobalParamControls : public Component
{
public:
    GlobalParamControls (AudioProcessorValueTreeState& vts,
                         chowdsp::HostContextProvider& hostContextProvider,
                         chowdsp::VariableOversampling<float>& oversampling);

    void resized() override;

private:
    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;

    struct SliderWithAttachment : public TextSlider
    {
        SliderWithAttachment (const chowdsp::FloatParameter* param, const chowdsp::HostContextProvider& hcp) : TextSlider (param, hcp) {}
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
