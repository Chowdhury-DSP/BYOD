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

    AudioProcessorEditor* createEditor() override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    ProcessorChain& getProcChain() { return procs; }
    AudioProcessorValueTreeState& getVTS() { return vts; }
    AudioProcessLoadMeasurer& getLoadMeasurer() { return loadMeasurer; }

private:
    ProcessorStore procStore;
    ProcessorChain procs;

    UndoManager undoManager { 300000 };

    AudioProcessLoadMeasurer loadMeasurer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYOD)
};
