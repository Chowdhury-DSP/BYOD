#pragma once

#include "../ProcessorStore.h"
#include "ChainIOProcessor.h"

#include "../utility/InputProcessor.h"
#include "../utility/OutputProcessor.h"

class ProcessorChainActionHelper;
class ProcessorChainPortMagnitudesHelper;
class ProcessorChainStateHelper;
class ProcessorChain : private AudioProcessorValueTreeState::Listener
{
    // clang-format off
    CREATE_LISTENER (
        Listener,
        listeners,
        virtual void processorAdded (BaseProcessor* /*proc*/) {}\
        virtual void processorRemoved (const BaseProcessor* /*proc*/) {}\
        virtual void refreshConnections() {}\
        virtual void connectionAdded (const ConnectionInfo& /*info*/) {}\
        virtual void connectionRemoved (const ConnectionInfo& /*info*/) {}\
    )
    // clang-format on
public:
    ProcessorChain (ProcessorStore& store,
                    AudioProcessorValueTreeState& vts,
                    std::unique_ptr<chowdsp::PresetManager>& presetMgr,
                    std::function<void (int)>&& latencyChangedCallback);
    ~ProcessorChain() override;

    static void createParameters (Parameters& params);
    void prepare (double sampleRate, int samplesPerBlock);
    void processAudio (AudioBuffer<float>& buffer);

    auto& getProcessors() { return procs; }
    const auto& getProcessors() const { return procs; }
    ProcessorStore& getProcStore() { return procStore; }

    InputProcessor& getInputProcessor() { return inputProcessor; }
    OutputProcessor& getOutputProcessor() { return outputProcessor; }

    auto& getActionHelper() { return *actionHelper; }
    auto& getStateHelper() { return *stateHelper; }
    auto& getOversampling() { return ioProcessor.getOversampling(); }

private:
    void initializeProcessors();
    void runProcessor (BaseProcessor* proc, AudioBuffer<float>& buffer, bool& outProcessed);
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    double mySampleRate = 48000.0;
    int mySamplesPerBlock = 512;

    OwnedArray<BaseProcessor> procs;
    ProcessorStore& procStore;
    UndoManager* um;

    InputProcessor inputProcessor;
    AudioBuffer<float> inputBuffer;
    OutputProcessor outputProcessor;
    ChainIOProcessor ioProcessor;

    std::unique_ptr<chowdsp::PresetManager>& presetManager;

    friend class ProcChainActions;
    friend class AddOrRemoveProcessor;
    friend class AddOrRemoveConnection;

    friend class ProcessorChainActionHelper;
    std::unique_ptr<ProcessorChainActionHelper> actionHelper;

    friend class ProcessorChainStateHelper;
    std::unique_ptr<ProcessorChainStateHelper> stateHelper;

    friend class ProcessorChainPortMagnitudesHelper;
    std::unique_ptr<ProcessorChainPortMagnitudesHelper> portMagsHelper;

    std::atomic_bool wasProcessCalled { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChain)
};
