#include "ProcessorChainActions.h"
#include "ProcessorChainActionHelper.h"

namespace ProcessorChainHelpers
{
void removeOutputConnectionsFromProcessor (ProcessorChain& chain, BaseProcessor* proc, UndoManager* um)
{
    for (int portIndex = 0; portIndex < proc->getNumOutputs(); ++portIndex)
    {
        auto numOutputConnections = proc->getNumOutputConnections (portIndex);
        while (numOutputConnections > 0)
        {
            auto connection = proc->getOutputConnection (portIndex, numOutputConnections - 1);
            um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
            numOutputConnections = proc->getNumOutputConnections (portIndex);
        }
    }
}
} // namespace ProcessorChainHelpers

class ProcChainActions
{
public:
    static void addProcessor (ProcessorChain& chain, BaseProcessor::Ptr newProc)
    {
        Logger::writeToLog (String ("Creating processor: ") + newProc->getName());

        newProc->playheadHelpers = &chain.playheadHelper;
        auto osFactor = chain.ioProcessor.getOversamplingFactor();
        newProc->prepareProcessing (osFactor * chain.mySampleRate, osFactor * chain.mySamplesPerBlock);

        BaseProcessor* newProcPtr = nullptr;
        {
            SpinLock::ScopedLockType scopedProcessingLock { chain.processingLock };
            newProcPtr = chain.procs.add (std::move (newProc));
        }

        for (auto* param : newProcPtr->getParameters())
        {
            if (auto* paramCast = dynamic_cast<juce::RangedAudioParameter*> (param))
                newProcPtr->getVTS().addParameterListener (paramCast->paramID, &chain);
        }

        chain.processorAddedBroadcaster (newProcPtr);
    }

    static void removeProcessor (ProcessorChain& chain, BaseProcessor* procToRemove, BaseProcessor::Ptr& saveProc)
    {
        Logger::writeToLog (String ("Removing processor: ") + procToRemove->getName());

        ProcessorChainHelpers::removeOutputConnectionsFromProcessor (chain, procToRemove, chain.um);

        chain.processorRemovedBroadcaster (procToRemove);

        for (auto* param : procToRemove->getParameters())
        {
            if (auto* paramCast = dynamic_cast<juce::RangedAudioParameter*> (param))
                procToRemove->getVTS().removeParameterListener (paramCast->paramID, &chain);
        }

        {
            SpinLock::ScopedLockType scopedProcessingLock { chain.processingLock };
            saveProc.reset (chain.procs.removeAndReturn (chain.procs.indexOf (procToRemove)));
        }
        saveProc->freeInternalMemory();
    }

    static void addConnection (ProcessorChain& chain, const ConnectionInfo& info)
    {
        Logger::writeToLog (String ("Adding connection from ") + info.startProc->getName() + ", port #"
                            + String (info.startPort) + " to " + info.endProc->getName() + " port #"
                            + String (info.endPort));

        {
            SpinLock::ScopedLockType scopedProcessingLock { chain.processingLock };
            info.startProc->addConnection (ConnectionInfo (info));
        }
        chain.connectionAddedBroadcaster (info);
    }

    static void removeConnection (ProcessorChain& chain, const ConnectionInfo& info)
    {
        Logger::writeToLog (String ("Removing connection from ") + info.startProc->getName() + ", port #"
                            + String (info.startPort) + " to " + info.endProc->getName() + " port #"
                            + String (info.endPort));

        {
            SpinLock::ScopedLockType scopedProcessingLock { chain.processingLock };
            info.startProc->removeConnection (info);
        }
        chain.connectionRemovedBroadcaster (info);
    }

    static bool getPresetWasDirty (ProcessorChain& chain)
    {
        if (chain.presetManager == nullptr)
            return true;

        return chain.presetManager->getIsDirty();
    }

private:
    ProcChainActions() = default; // static use only!
};

//=========================================================
AddOrRemoveProcessor::AddOrRemoveProcessor (ProcessorChain& procChain, BaseProcessor::Ptr newProc) : chain (procChain),
                                                                                                     actionProc (std::move (newProc)),
                                                                                                     isRemoving (false),
                                                                                                     wasDirty (ProcChainActions::getPresetWasDirty (chain))
{
}

AddOrRemoveProcessor::AddOrRemoveProcessor (ProcessorChain& procChain, BaseProcessor* procToRemove) : chain (procChain),
                                                                                                      actionProcPtr (procToRemove),
                                                                                                      isRemoving (true),
                                                                                                      wasDirty (ProcChainActions::getPresetWasDirty (chain))
{
}

template <typename PointerType>
bool waitForPointerCheck (const PointerType& pointer, int waitCycles = 6)
{
    int count = 0;
    while (pointer == nullptr)
    {
        if (count >= waitCycles)
            return false;

        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        count++;
    }

    return true;
}

bool AddOrRemoveProcessor::perform()
{
    if (isRemoving)
    {
        if (! waitForPointerCheck (actionProcPtr))
            return false;

        jassert (actionProcPtr != nullptr);
        ProcChainActions::removeProcessor (chain, actionProcPtr, actionProc);
    }
    else
    {
        if (! waitForPointerCheck (actionProc))
            return false;

        jassert (actionProc != nullptr);
        actionProcPtr = actionProc.get();
        ProcChainActions::addProcessor (chain, std::move (actionProc));
    }

    if (! wasDirty)
        chain.presetManager->setIsDirty (true);

    return true;
}

bool AddOrRemoveProcessor::undo()
{
    if (isRemoving)
    {
        if (! waitForPointerCheck (actionProc))
            return false;

        jassert (actionProc != nullptr);
        actionProcPtr = actionProc.get();
        ProcChainActions::addProcessor (chain, std::move (actionProc));
    }
    else
    {
        if (! waitForPointerCheck (actionProcPtr))
            return false;

        jassert (actionProcPtr != nullptr);
        ProcChainActions::removeProcessor (chain, actionProcPtr, actionProc);
    }

    if (! wasDirty)
        chain.presetManager->setIsDirty (false);

    return true;
}

//=========================================================
AddOrRemoveConnection::AddOrRemoveConnection (ProcessorChain& procChain, ConnectionInfo&& cInfo, bool removing) : chain (procChain),
                                                                                                                  info (cInfo),
                                                                                                                  isRemoving (removing),
                                                                                                                  wasDirty (ProcChainActions::getPresetWasDirty (chain))
{
}

bool AddOrRemoveConnection::perform()
{
    if (isRemoving)
    {
        ProcChainActions::removeConnection (chain, info);
    }
    else
    {
        ProcChainActions::addConnection (chain, info);
    }

    if (! wasDirty)
        chain.presetManager->setIsDirty (true);

    return true;
}

bool AddOrRemoveConnection::undo()
{
    if (isRemoving)
    {
        ProcChainActions::addConnection (chain, info);
    }
    else
    {
        ProcChainActions::removeConnection (chain, info);
    }

    if (! wasDirty)
        chain.presetManager->setIsDirty (false);

    return true;
}
