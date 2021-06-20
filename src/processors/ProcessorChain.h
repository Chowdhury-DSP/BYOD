#pragma once

#include "ProcessorStore.h"

class ProcessorChain
{
    CREATE_LISTENER (Listener, listeners,
        virtual void processorAdded (BaseProcessor* /*proc*/) {}\
    )
public:
    ProcessorChain (ProcessorStore& store);

    void prepare (double sampleRate, int samplesPerBlock);
    void processAudio (AudioBuffer<float> buffer);

    void addProcessor (BaseProcessor::Ptr newProc);

    ProcessorStore& getProcStore() { return procStore; }

private:
    double mySampleRate = 48000.0;
    int mySamplesPerBlock = 512;

    OwnedArray<BaseProcessor> procs;
    ProcessorStore& procStore;

    SpinLock processingLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChain)
};
