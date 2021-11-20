#pragma once

#include "../ProcessorStore.h"
#include "DryWetProcessor.h"

#include "../utility/InputProcessor.h"
#include "../utility/OutputProcessor.h"

class ProcessorChain : private AudioProcessorValueTreeState::Listener
{
    // clang-format off
    CREATE_LISTENER (
        Listener,
        listeners,
        virtual void processorAdded (BaseProcessor* /*proc*/) {}\
        virtual void processorPrepareToRemove (const BaseProcessor* /*proc*/) {}\
        virtual void processorRemoved (const BaseProcessor* /*proc*/) {}\
        virtual void refreshConnections() {}\
        virtual void connectionAdded (const ConnectionInfo& /*info*/) {}\
        virtual void connectionRemoved (const ConnectionInfo& /*info*/) {}\
    )
    // clang-format on
public:
    ProcessorChain (ProcessorStore& store, AudioProcessorValueTreeState& vts, std::unique_ptr<chowdsp::PresetManager>& presetMgr);
    ~ProcessorChain();

    static void createParameters (Parameters& params);
    void prepare (double sampleRate, int samplesPerBlock);
    void processAudio (AudioBuffer<float> buffer);

    OwnedArray<BaseProcessor>& getProcessors() { return procs; }
    ProcessorStore& getProcStore() { return procStore; }
    const SpinLock& getProcChainLock() const { return processingLock; }

    InputProcessor& getInputProcessor() { return inputProcessor; }
    OutputProcessor& getOutputProcessor() { return outputProcessor; }

    auto& getActionHelper() { return *actionHelper; }
    auto& getStateHelper() { return *stateHelper; }

private:
    void initializeProcessors (int curOS);
    void runProcessor (BaseProcessor* proc, AudioBuffer<float>& buffer, bool& outProcessed);
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    double mySampleRate = 48000.0;
    int mySamplesPerBlock = 512;

    OwnedArray<BaseProcessor> procs;
    ProcessorStore& procStore;
    SpinLock processingLock;
    UndoManager* um;

    InputProcessor inputProcessor;
    AudioBuffer<float> inputBuffer;
    OutputProcessor outputProcessor;

    std::atomic<float>* oversamplingParam = nullptr;
    std::unique_ptr<dsp::Oversampling<float>> overSample[5];
    int prevOS = 0;

    std::atomic<float>* inGainParam = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    dsp::Gain<float> inGain, outGain;

    std::atomic<float>* dryWetParam = nullptr;
    DryWetProcessor dryWetMixer;

    std::unique_ptr<chowdsp::PresetManager>& presetManager;

    friend class ProcChainActions;
    friend class AddOrRemoveProcessor;
    friend class AddOrRemoveConnection;

    friend class ProcessorChainActionHelper;
    std::unique_ptr<ProcessorChainActionHelper> actionHelper;

    friend class ProcessorChainStateHelper;
    std::unique_ptr<ProcessorChainStateHelper> stateHelper;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChain)
};
