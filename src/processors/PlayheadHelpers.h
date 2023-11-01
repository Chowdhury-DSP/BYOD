#pragma once

#include <pch.h>

struct PlayheadHelpers
{
    static void prepare(double sampleRate, int maxBlockSize)
    {
        ignoreUnused(sampleRate, maxBlockSize);
    }

    void process(AudioPlayHead* playHead, int numSamples)
    {
        ignoreUnused(numSamples);
        if (playHead != nullptr)
        {
            info = *playHead->getPosition();
            bpm.store(*info.getBpm());
        }
    }

    std::atomic<double> bpm;
    AudioPlayHead::PositionInfo info;
};

