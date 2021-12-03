#pragma once

#include <pch.h>

class Port : public Component
{
    // clang-format off
    CREATE_LISTENER (
        PortListener,
        portListeners,
        virtual void createCable (Port* /*origin*/, const MouseEvent&) {}\
        virtual void refreshCable (const MouseEvent&) {}\
        virtual void releaseCable (const MouseEvent&) {}\
        virtual void destroyCable (Port* /*origin*/) {}\
    )
    // clang-format on
public:
    Port() = default;

    void setInputOutput (bool shouldBeInput) { isInput = shouldBeInput; }
    void setConnectionStatus (bool connectionStatus);

    void paint (Graphics& g) override;

    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

private:
    bool isInput = false;
    bool isConnected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Port)
};
