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

    void paint (Graphics& g) override
    {
        auto portBounds = getLocalBounds().toFloat();
        g.setColour (isInput ? Colours::black : Colours::darkgrey);
        g.fillEllipse (portBounds);

        g.setColour (Colours::lightgrey);
        g.drawEllipse (portBounds.reduced (1.0f), 1.0f);
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (isInput)
            portListeners.call (&PortListener::destroyCable, this);
        else
            portListeners.call (&PortListener::createCable, this, e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (! isInput)
            portListeners.call (&PortListener::refreshCable, e);
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (! isInput)
            portListeners.call (&PortListener::releaseCable, e);
    }

private:
    bool isInput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Port)
};
