#pragma once

#include "processors/ProcessorChain.h"
#include "processors/ProcessorStore.h"

class BYOD : public chowdsp::PluginBase<BYOD>
{
public:
    BYOD();

    static void addParameters (Parameters& params);
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processAudioBlock (AudioBuffer<float>& buffer) override;

    ProcessorChain& getProcChain() { return procs; }
    AudioProcessorEditor* createEditor() override;

private:
    ProcessorStore procStore;
    ProcessorChain procs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYOD)
};
