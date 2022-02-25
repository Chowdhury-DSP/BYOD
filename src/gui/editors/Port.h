#pragma once

#include <pch.h>

class Port : public Component
{
public:
    Port() = default;

    void setInputOutput (bool shouldBeInput) { isInput = shouldBeInput; }
    void setConnectionStatus (bool connectionStatus);

    void paint (Graphics& g) override;

private:
    bool isInput = false;
    bool isConnected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Port)
};
