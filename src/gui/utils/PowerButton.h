#pragma once

#include <pch.h>

using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

class PowerButton : public Component
{
public:
    explicit PowerButton (const Colour& powerColour) : button ("", DrawableButton::ImageStretched)
    {
        button.setColour (DrawableButton::backgroundColourId, Colours::transparentBlack);
        button.setColour (DrawableButton::backgroundOnColourId, Colours::transparentBlack);

        addAndMakeVisible (button);
        button.setClickingTogglesState (true);

        auto onImage = Drawable::createFromImageData (BinaryData::PowerButton_svg, BinaryData::PowerButton_svgSize);
        auto offImage = onImage->createCopy();

        onImage->replaceColour (Colours::black, powerColour);
        offImage->replaceColour (Colours::black, Colours::darkgrey);
        button.setImages (offImage.get(), offImage.get(), onImage.get(), offImage.get(), onImage.get(), onImage.get(), offImage.get());

        button.onStateChange = [=]
        { enableDisable(); };
    }

    void resized() override
    {
        auto dim = int ((float) jmin (getWidth(), getHeight()) * 0.6f);
        auto centre = getLocalBounds().getCentre();
        auto topLeft = centre.translated (-dim / 2, -dim / 2);

        button.setBounds (topLeft.x, topLeft.y, dim, dim);
    }

    void attachButton (AudioProcessorValueTreeState& vts, const String& paramID)
    {
        powerAttach = std::make_unique<ButtonAttachment> (vts, paramID, button);
    }

    void setEnableDisableComps (const std::initializer_list<Component*>& comps)
    {
        enableDisableComps = Array<Component*> (comps);
        enableDisable();
    }

    void enableDisable()
    {
        bool shouldBeEnabled = button.getToggleState();

        for (auto* c : enableDisableComps)
            c->setEnabled (shouldBeEnabled);
    }

private:
    DrawableButton button;
    std::unique_ptr<ButtonAttachment> powerAttach;

    Array<Component*> enableDisableComps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerButton)
};
