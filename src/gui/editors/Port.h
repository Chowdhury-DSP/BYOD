#pragma once

#include <pch.h>

class Port : public Component
{
public:
    explicit Port (const Colour& processorColour);

    void setInputOutput (bool shouldBeInput) { isInput = shouldBeInput; }
    void setConnectionStatus (bool connectionStatus);

    void paint (Graphics& g) override;

private:
    bool isInput = false;
    bool isConnected = false;

    const Colour& procColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Port)
};
