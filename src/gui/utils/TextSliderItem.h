#pragma once

#include "TextSlider.h"

class TextSliderItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY (TextSliderItem)

    TextSliderItem (foleys::MagicGUIBuilder& builder, const ValueTree& node) : foleys::GuiItem (builder, node)
    {
        setColourTranslation (
            { { "slider-background", juce::Slider::backgroundColourId },
              { "slider-thumb", juce::Slider::thumbColourId },
              { "slider-track", juce::Slider::trackColourId },
              { "rotary-fill", juce::Slider::rotarySliderFillColourId },
              { "rotary-outline", juce::Slider::rotarySliderOutlineColourId },
              { "slider-text", juce::Slider::textBoxTextColourId },
              { "slider-text-background", juce::Slider::textBoxBackgroundColourId },
              { "slider-text-highlight", juce::Slider::textBoxHighlightColourId },
              { "slider-text-outline", juce::Slider::textBoxOutlineColourId } });

        addAndMakeVisible (slider);
    }

    void update() override
    {
        attachment.reset();

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);

        auto paramID = configNode.getProperty (foleys::IDs::parameter, juce::String()).toString();
        if (paramID.isNotEmpty())
            attachment = getMagicState().createAttachment (paramID, slider);
    }

    std::vector<foleys::SettableProperty> getSettableProperties() const override
    {
        std::vector<foleys::SettableProperty> itemProperties;

        itemProperties.push_back ({ configNode, foleys::IDs::parameter, foleys::SettableProperty::Choice, {}, magicBuilder.createParameterMenuLambda() });

        return itemProperties;
    }

    juce::Component* getWrappedComponent() override
    {
        return &slider;
    }

private:
    TextSlider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextSliderItem)
};
