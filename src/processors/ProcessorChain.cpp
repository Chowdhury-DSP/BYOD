#include "ProcessorChain.h"

ProcessorChain::ProcessorChain (ProcessorStore& store) : procStore (store)
{
    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChain::addProcessor, this, _1);
}

void ProcessorChain::prepare (double sampleRate, int samplesPerBlock)
{
    mySampleRate = sampleRate;
    mySamplesPerBlock = samplesPerBlock;

    for (auto* processor : procs)
        processor->prepare (sampleRate, samplesPerBlock);
}

void ProcessorChain::processAudio (AudioBuffer<float> buffer)
{
    SpinLock::ScopedTryLockType tryProcessingLock (processingLock);
    if (! tryProcessingLock.isLocked())
        return;

    for (auto* processor : procs)
        processor->processAudio (buffer);
}

void ProcessorChain::addProcessor (BaseProcessor::Ptr newProc)
{
    DBG (String ("Creating processor: ") + newProc->getName());

    newProc->prepare (mySampleRate, mySamplesPerBlock);
    
    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
    auto* newProcPtr = procs.add (std::move (newProc));

    listeners.call (&Listener::processorAdded, newProcPtr);
}
