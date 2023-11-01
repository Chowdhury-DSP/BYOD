#pragma once

#include <pch.h>

struct PlayheadHelpers
{
    void process (AudioPlayHead* playHead, int numSamples)
    {
        ignoreUnused (numSamples);
        if (playHead != nullptr)
        {
            bpm.store (playHead->getPosition().orFallback (AudioPlayHead::PositionInfo {}).getBpm().orFallback (120.0));
        }
        else
        {
            bpm.store (120.0);
        }
    }

    std::atomic<double> bpm;
};
