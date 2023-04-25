#pragma once

#include "processors/ProcessorStore.h"
#include "processors/chain/ProcessorChain.h"
#include "state/ParamForwardManager.h"
#include "state/StateManager.h"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wattributes", "-Woverloaded-virtual")

class BYOD : public chowdsp::PluginBase<BYOD>
{
public:
    BYOD();

    static void addParameters (Parameters& params);
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (AudioBuffer<float>& buffer) override;
    void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    AudioProcessorEditor* createEditor() override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    auto& getProcChain() { return *procs; }
    auto& getVTS() { return vts; }
    const auto& getVTS() const { return vts; }
    const auto& getParamForwarder() const { return *paramForwarder; }
    auto& getOversampling() { return procs->getOversampling(); }
    auto& getLoadMeasurer() { return loadMeasurer; }
    auto* getOpenGLHelper() { return openGLHelper.get(); }
    auto& getUndoManager() { return undoManager; }
    auto& getStateManager() { return *stateManager; }

private:
    void processBypassDelay (AudioBuffer<float>& buffer);
    void updateSampleLatency (int latencySamples);

    std::optional<File> crashLogFile;
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

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
