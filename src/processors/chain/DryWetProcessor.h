#pragma once

#include <pch.h>

/** Simple processor to mix dry and wet signals */
class DryWetProcessor
{
public:
    DryWetProcessor() = default;

    void setDryWet (float newDryWet) { dryWet = newDryWet; }
    float getDryWet() const noexcept { return dryWet; }

    void prepare (const dsp::ProcessSpec& spec);
    void reset();
    void copyDryBuffer (const AudioBuffer<float>& buffer);

    /** Mix dry and wet buffer. */
    void processBlock (AudioBuffer<float>& buffer, int latencySamples);

private:
    float dryWet = 0.0f;
    float lastDryWet = 0.0f;

    AudioBuffer<float> dryBuffer;

    using DelayType = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::None>;
    DelayType dryDelay { 1 << 10 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DryWetProcessor)
};
