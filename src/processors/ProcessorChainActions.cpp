#include "ProcessorChainActions.h"

class ProcChainActions
{
public:
    static BaseProcessor* addProcessor (ProcessorChain& chain, BaseProcessor::Ptr newProc)
    {
        DBG (String ("Creating processor: ") + newProc->getName());

        newProc->prepare (chain.mySampleRate, chain.mySamplesPerBlock);

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

    static void moveProcessor (ProcessorChain& chain, int indexToMove, int slotIndex)
    {
        chain.procs.move (indexToMove, slotIndex);
        chain.listeners.call (&ProcessorChain::Listener::processorMoved, indexToMove, slotIndex);
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

//=========================================================
MoveProcessor::MoveProcessor (ProcessorChain& procChain, int indexToMove, int slotIndex) : chain (procChain),
                                                                                           startIndex (indexToMove),
                                                                                           endIndex (slotIndex)
{
}

bool MoveProcessor::perform()
{
    ProcChainActions::moveProcessor (chain, startIndex, endIndex);
    return true;
}

bool MoveProcessor::undo()
{
    ProcChainActions::moveProcessor (chain, endIndex, startIndex);
    return true;
}

UndoableAction* MoveProcessor::createCoalescedAction (UndoableAction* nextAction)
{
    if (auto* next = dynamic_cast<MoveProcessor*> (nextAction))
        if (&(next->chain) == &chain && next->startIndex == endIndex)
            return new MoveProcessor (chain, startIndex, next->endIndex);

    return nullptr;
}
