#pragma once

#include "ProcessorStore.h"

class ProcessorChain
{
    CREATE_LISTENER (Listener, listeners,
        virtual void processorAdded (BaseProcessor* proc) {}\
    )
public:
    ProcessorChain (ProcessorStore& store);

    ProcessorStore& getProcStore() { return procStore; }

    void addProcessor (BaseProcessor::Ptr newProc);

private:
    OwnedArray<BaseProcessor> procs;
    ProcessorStore& procStore;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChain)
};
