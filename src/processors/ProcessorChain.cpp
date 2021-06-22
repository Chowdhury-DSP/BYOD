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
    {
        if (processor->isBypassed())
        {
            processor->processAudioBypassed (buffer);
            continue;
        }

        processor->processAudio (buffer);
    }
}

void ProcessorChain::addProcessor (BaseProcessor::Ptr newProc)
{
    DBG (String ("Creating processor: ") + newProc->getName());

    newProc->prepare (mySampleRate, mySamplesPerBlock);

    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
    auto* newProcPtr = procs.add (std::move (newProc));

    listeners.call (&Listener::processorAdded, newProcPtr);
}

void ProcessorChain::removeProcessor (BaseProcessor* procToRemove)
{
    DBG (String ("Removing processor: ") + procToRemove->getName());

    listeners.call (&Listener::processorRemoved, procToRemove);

    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
    procs.removeObject (procToRemove);
}

std::unique_ptr<XmlElement> ProcessorChain::saveProcChain()
{
    auto xml = std::make_unique<XmlElement> ("proc_chain");
    for (auto* proc : procs)
    {
        auto procXml = std::make_unique<XmlElement> (proc->getName().replaceCharacter (' ', '_'));
        procXml->addChildElement (proc->toXML().release());
        xml->addChildElement (procXml.release());
    }

    return std::move (xml);
}

void ProcessorChain::loadProcChain (XmlElement* xml)
{
    while (! procs.isEmpty())
        removeProcessor (procs.getLast());

    for (auto* procXml : xml->getChildIterator())
    {
        auto newProc = procStore.createProcByName (procXml->getTagName().replaceCharacter ('_', ' '));
        if (newProc == nullptr)
        {
            jassertfalse;
            continue;
        }
        
        if (procXml->getNumChildElements() > 0)
            newProc->fromXML (procXml->getChildElement (0));

        addProcessor (std::move (newProc));
    }
}
