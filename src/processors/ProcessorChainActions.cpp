#include "ProcessorChainActions.h"

class ProcChainActions
{
public:
    static BaseProcessor* addProcessor (ProcessorChain& chain, BaseProcessor::Ptr newProc)
    {
        DBG (String ("Creating processor: ") + newProc->getName());

        int curOS = static_cast<int> (*chain.oversamplingParam);
        auto osFactor = (int) chain.overSample[curOS]->getOversamplingFactor();
        newProc->prepare (osFactor * chain.mySampleRate, osFactor * chain.mySamplesPerBlock);

        SpinLock::ScopedLockType scopedProcessingLock (chain.processingLock);
        auto* newProcPtr = chain.procs.add (std::move (newProc));

        chain.listeners.call (&ProcessorChain::Listener::processorAdded, newProcPtr);
        return newProcPtr;
    }

    static void removeProcessor (ProcessorChain& chain, const BaseProcessor* procToRemove)
    {
        DBG (String ("Removing processor: ") + procToRemove->getName());

        chain.listeners.call (&ProcessorChain::Listener::processorRemoved, procToRemove);

        SpinLock::ScopedLockType scopedProcessingLock (chain.processingLock);
        chain.procs.removeObject (procToRemove, false);
    }

private:
    ProcChainActions() {} // static use only!
};

//=========================================================
AddOrRemoveProcessor::AddOrRemoveProcessor (ProcessorChain& procChain, BaseProcessor::Ptr newProc) : chain (procChain),
                                                                                                     actionProc (std::move (newProc)),
                                                                                                     isRemoving (false)
{
}

AddOrRemoveProcessor::AddOrRemoveProcessor (ProcessorChain& procChain, BaseProcessor* procToRemove) : chain (procChain),
                                                                                                      actionProcPtr (procToRemove),
                                                                                                      isRemoving (true)
{
}

bool AddOrRemoveProcessor::perform()
{
    if (isRemoving)
    {
        jassert (actionProcPtr != nullptr);

        ProcChainActions::removeProcessor (chain, actionProcPtr);
        actionProc.reset (actionProcPtr);
    }
    else
    {
        jassert (actionProc != nullptr);

        actionProcPtr = ProcChainActions::addProcessor (chain, std::move (actionProc));
    }

    return true;
}

bool AddOrRemoveProcessor::undo()
{
    if (isRemoving)
    {
        jassert (actionProc != nullptr);

        actionProcPtr = ProcChainActions::addProcessor (chain, std::move (actionProc));
    }
    else
    {
        jassert (actionProcPtr != nullptr);

        ProcChainActions::removeProcessor (chain, actionProcPtr);
        actionProc.reset (actionProcPtr);
    }

    return true;
}
