#pragma once

#include "ProcessorStore.h"

class ProcessorChain
{
    CREATE_LISTENER (Listener, listeners,
        virtual void processorAdded (BaseProcessor* /*proc*/) {}\
        virtual void processorRemoved (const BaseProcessor* /*proc*/) {}\
        virtual void processorMoved (int /*procToMove*/, int /*procInSlot*/) {}\
    )
public:
    ProcessorChain (ProcessorStore& store);

    void prepare (double sampleRate, int samplesPerBlock);
    void processAudio (AudioBuffer<float> buffer);

    void addProcessor (BaseProcessor::Ptr newProc);
    void removeProcessor (const BaseProcessor* procToRemove);
    void moveProcessor (const BaseProcessor* procToMove, const BaseProcessor* procInSlot);
    OwnedArray<BaseProcessor>& getProcessors() { return procs; }

    std::unique_ptr<XmlElement> saveProcChain();
    void loadProcChain (XmlElement* xml);

    ProcessorStore& getProcStore() { return procStore; }
    const SpinLock& getProcChainLock() const { return processingLock; }

private:
    double mySampleRate = 48000.0;
    int mySamplesPerBlock = 512;

    OwnedArray<BaseProcessor> procs;
    ProcessorStore& procStore;

    SpinLock processingLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChain)
};
