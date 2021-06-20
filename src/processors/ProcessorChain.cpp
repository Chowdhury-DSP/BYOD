#include "ProcessorChain.h"

ProcessorChain::ProcessorChain (ProcessorStore& store) : procStore (store)
{
    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChain::addProcessor, this, _1);
}

void ProcessorChain::addProcessor (BaseProcessor::Ptr newProc)
{
    DBG (String ("Creating processor: ") + newProc->getName());

    auto* newProcPtr = procs.add (std::move (newProc));
    listeners.call (&Listener::processorAdded, newProcPtr);
}
