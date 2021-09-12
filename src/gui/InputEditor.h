#pragma once

#include <pch.h>

class InputEditor : public Component
{
public:
    InputEditor() = default;

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::orange);

        g.setColour (Colours::black);
        g.setFont (Font (25.0f).boldened());
        g.drawFittedText ("Input", 5, 0, getWidth() - 50, 30, Justification::centredLeft, 1);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputEditor)
};

class OutputEditor : public Component
{
public:
    OutputEditor() = default;

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::lightskyblue);

        g.setColour (Colours::black);
        g.setFont (Font (25.0f).boldened());
        g.drawFittedText ("Output", 5, 0, getWidth() - 50, 30, Justification::centredLeft, 1);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputEditor)
};
