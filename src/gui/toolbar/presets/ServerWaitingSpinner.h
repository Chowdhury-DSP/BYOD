#if BYOD_BUILD_PRESET_SERVER

#pragma once

#include <pch.h>

struct ServerWaitingSpinner : public Component,
                              private Timer
{
    ServerWaitingSpinner() { startTimerHz (27); }
    void timerCallback() override { repaint(); }

    void paint (Graphics& g) override
    {
        getLookAndFeel().drawSpinningWaitAnimation (g, Colours::white, 0, 0, getWidth(), getHeight());
    }
};

#endif // BYOD_BUILD_PRESET_SERVER
