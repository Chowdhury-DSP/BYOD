#ifndef INPUTBUFFERPROCESSOR_H_INCLUDED
#define INPUTBUFFERPROCESSOR_H_INCLUDED

#include <pch.h>

class InputBufferProcessor : chowdsp::IIRFilter<1>
{
public:
    InputBufferProcessor() = default;

    void prepare (float sampleRate);
    void calcCoefs();
    void processBlock (float* buffer, const int numSamples) noexcept override;

private:
    float fs = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputBufferProcessor)
};

#endif // INPUTBUFFERPROCESSOR_H_INCLUDED
