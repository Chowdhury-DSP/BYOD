#pragma once

#include <pch.h>

class BigMuffClippingStage
{
public:
    BigMuffClippingStage() = default;

    void prepare (double sampleRate);
    void reset();

    template <bool highQuality>
    void processBlock (AudioBuffer<float>& buffer, const int smoothing) noexcept;

private:
    chowdsp::IIRFilter<1, float> inputFilter[2];

    float fs = 48000.0f;
    float y_1[2] {}; // newton-raphson state
    float C_12_1[2] {}; // capacitor C12 state

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigMuffClippingStage)
};
