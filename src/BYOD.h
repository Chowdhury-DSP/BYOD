#pragma once

#include "processors/ProcessorStore.h"
#include "processors/chain/ProcessorChain.h"
#include "state/ParamForwardManager.h"
#include "state/StateManager.h"

class BYOD : public chowdsp::PluginBase<BYOD>
#if HAS_CLAP_JUCE_EXTENSIONS
    ,
             private clap_juce_extensions::clap_properties
#endif
{
public:
    BYOD();

    static void addParameters (Parameters& params);
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processAudioBlock (AudioBuffer<float>& buffer) override;
    void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;
    String getWrapperTypeString() const;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    auto& getProcChain() { return *procs; }
    auto& getVTS() { return vts; }
    auto& getOversampling() { return procs->getOversampling(); }
    auto& getLoadMeasurer() { return loadMeasurer; }
    auto* getOpenGLHelper() { return openGLHelper.get(); }
    auto& getUndoManager() { return undoManager; }
    auto& getStateManager() { return *stateManager; }

private:
    void processBypassDelay (AudioBuffer<float>& buffer);
    void updateSampleLatency (int latencySamples);

    chowdsp::PluginLogger logger;
    chowdsp::SharedPluginSettings pluginSettings;
    [[maybe_unused]] chowdsp::SharedLNFAllocator lnfAllocator; // keep alive!

    ProcessorStore procStore;
    std::unique_ptr<ProcessorChain> procs;
    [[maybe_unused]] std::unique_ptr<ParamForwardManager> paramForwarder;

    AudioBuffer<float> bypassScratchBuffer;
    chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::None> bypassDelay { 1 << 10 };

    UndoManager undoManager { 500000 };

    AudioProcessLoadMeasurer loadMeasurer;

    std::unique_ptr<StateManager> stateManager;

    std::unique_ptr<chowdsp::OpenGLHelper> openGLHelper = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYOD)
};
