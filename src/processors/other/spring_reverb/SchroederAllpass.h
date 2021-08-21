#pragma once

#include <pch.h>

template <int order = 1>
class SchroederAllpass
{
public:
    SchroederAllpass() = default;

    void prepare (double sampleRate)
    {
        dsp::ProcessSpec spec { sampleRate, (uint32) 256, 1 };
        delay.prepare (spec);
        
        nestedAllpass.prepare (sampleRate);
    }

    void reset()
    {
        delay.reset();
        nestedAllpass.reset();
    }

    void setParams (float delaySamp, float feedback)
    {
        delay.setDelay (delaySamp);
        g = feedback;

        nestedAllpass.setParams (delaySamp, feedback);
    }

    inline float processSample (float x) noexcept
    {
        auto delayOut = nestedAllpass.processSample (delay.popSample (0));
        x += g * delayOut;
        delay.pushSample (0, x);
        return delayOut - g * x;
    }

private:
    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Thiran> delay { 1 << 18 };
    SchroederAllpass<order - 1> nestedAllpass;
    float g = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchroederAllpass)
};

template<>
class SchroederAllpass<1>
{
public:
    SchroederAllpass() = default;

    void prepare (double sampleRate)
    {
        dsp::ProcessSpec spec { sampleRate, (uint32) 256, 1 };
        delay.prepare (spec);
    }

    void reset()
    {
        delay.reset();
    }

    void setParams (float delaySamp, float feedback)
    {
        delay.setDelay (delaySamp);
        g = feedback;
    }

    inline float processSample (float x) noexcept
    {
        auto delayOut = delay.popSample (0);
        x += g * delayOut;
        delay.pushSample (0, x);
        return delayOut - g * x;
    }

private:
    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Thiran> delay { 1 << 18 };
    float g;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchroederAllpass)
};
