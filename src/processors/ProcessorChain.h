#pragma once

#include "DryWetProcessor.h"
#include "ProcessorStore.h"

#include "utility/InputProcessor.h"
#include "utility/OutputProcessor.h"

class ProcessorChain
{
    // clang-format off
    CREATE_LISTENER (
        Listener,
        listeners,
        virtual void processorAdded (BaseProcessor* /*proc*/) {}\
        virtual void processorRemoved (const BaseProcessor* /*proc*/) {}\
        virtual void processorMoved (int /*procToMove*/, int /*procInSlot*/) {}\
        virtual void refreshConnections () {}\
    )
    // clang-format on
public:
    ProcessorChain (ProcessorStore& store, AudioProcessorValueTreeState& vts);

    static void createParameters (Parameters& params);
    void prepare (double sampleRate, int samplesPerBlock);
    void processAudio (AudioBuffer<float> buffer);

    void addProcessor (BaseProcessor::Ptr newProc);
    void removeProcessor (BaseProcessor* procToRemove);
    void moveProcessor (const BaseProcessor* procToMove, const BaseProcessor* procInSlot);
    OwnedArray<BaseProcessor>& getProcessors() { return procs; }

    std::unique_ptr<XmlElement> saveProcChain();
    void loadProcChain (XmlElement* xml);

    ProcessorStore& getProcStore() { return procStore; }
    const SpinLock& getProcChainLock() const { return processingLock; }

    InputProcessor& getInputProcessor() { return inputProcessor; }
    OutputProcessor& getOutputProcessor() { return outputProcessor; }

private:
    void initializeProcessors (int curOS);
    void runProcessor (BaseProcessor* proc, AudioBuffer<float>& buffer, bool& outProcessed);

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

    friend class ProcChainActions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChain)
};
