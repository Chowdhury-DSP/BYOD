#pragma once

#include "processors/ProcessorStore.h"
#include "processors/chain/ProcessorChain.h"
#include "state/ParamForwardManager.h"

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
    chowdsp::PluginLogger logger;
    chowdsp::SharedPluginSettings pluginSettings;

    ProcessorStore procStore;
    ProcessorChain procs;
    ParamForwardManager paramForwarder;

    UndoManager undoManager { 500000 };

    AudioProcessLoadMeasurer loadMeasurer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYOD)
};
