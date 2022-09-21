#pragma once

#include <pch.h>

enum PortType
{
    regular,
    modulation
};

class Port : public Component
{
public:
    Port (const Colour& processorColour, const PortType type);

    void setInputOutput (bool shouldBeInput) { isInput = shouldBeInput; }
    void setConnectionStatus (bool connectionStatus);

    void paint (Graphics& g) override;

private:
    Colour getPortColour();

    bool isInput = false;
    bool isConnected = false;

    const Colour& procColour;
    const PortType portType = regular;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Port)
};
