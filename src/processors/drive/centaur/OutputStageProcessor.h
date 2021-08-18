#ifndef OUTPOUTSTAGEPROCESSOR_H_INCLUDED
#define OUTPOUTSTAGEPROCESSOR_H_INCLUDED

#include <pch.h>

class OutputStageProc : public chowdsp::IIRFilter<1>
{
public:
    OutputStageProc()
    {
        levelSmooth = 1.0f;
    }

    void setLevel (float level)
    {
        levelSmooth.setTargetValue (jlimit (0.00001f, 1.0f, level));
    }

    void prepare (float sampleRate);
    void calcCoefs (float curLevel);
    void processBlock (float* block, const int numSamples) noexcept override;

private:
    float fs = 44100.0f;

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> levelSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStageProc)
};

#endif // OUTPOUTSTAGEPROCESSOR_H_INCLUDED
