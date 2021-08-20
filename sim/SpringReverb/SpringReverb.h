#pragma once

#include <JuceHeader.h>

class SpringReverb
{
public:
    SpringReverb() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void processBlock (AudioBuffer<float>& buffer);

private:


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringReverb)
};
