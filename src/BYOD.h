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

    auto& getProcChain() { return *procs; }
    auto& getVTS() { return vts; }
    auto& getLoadMeasurer() { return loadMeasurer; }
    auto& getOpenGLHelper() { return openGLHelper; }

private:
    chowdsp::PluginLogger logger;
    chowdsp::SharedPluginSettings pluginSettings;
    [[maybe_unused]] chowdsp::SharedLNFAllocator lnfAllocator; // keep alive!

    ProcessorStore procStore;
    std::unique_ptr<ProcessorChain> procs;
    [[maybe_unused]] std::unique_ptr<ParamForwardManager> paramForwarder;

    UndoManager undoManager { 500000 };

    AudioProcessLoadMeasurer loadMeasurer;

    chowdsp::OpenGLHelper openGLHelper;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYOD)
};
