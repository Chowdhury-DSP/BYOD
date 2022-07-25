#include "ProcessorChainActions.h"
#include "ProcessorChainActionHelper.h"

namespace ProcessorChainHelpers
{
void removeOutputConnectionsFromProcessor (ProcessorChain& chain, BaseProcessor* proc, UndoManager* um)
{
    for (int portIndex = 0; portIndex < proc->getNumOutputs(); ++portIndex)
    {
        auto numOutputConnections = proc->getNumOutputConnections (0);
        while (numOutputConnections > 0)
        {
            auto connection = proc->getOutputConnection (0, numOutputConnections - 1);
            um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
            numOutputConnections = proc->getNumOutputConnections (0);
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

        auto* newProcPtr = newProc.get();

        auto osFactor = chain.ioProcessor.getOversamplingFactor();
        newProc->prepareProcessing (osFactor * chain.mySampleRate, osFactor * chain.mySamplesPerBlock);

        std::atomic_bool hasProcBeenAdded { false };
        chain.getActionHelper().actionQueue.emplace (
            [&chain, &hasProcBeenAdded, p = std::move (newProc)]() mutable
            {
                chain.procs.add (std::move (p));
                hasProcBeenAdded = true;
            });

        // Ideally we wouldn't be blocking the message thread here, but unfortunately
        // the undo manager will get out of sync if we don't!
        while (! hasProcBeenAdded)
            Thread::sleep (4);

        for (auto* param : newProcPtr->getParameters())
        {
            if (auto* paramCast = dynamic_cast<juce::RangedAudioParameter*> (param))
                newProcPtr->getVTS().addParameterListener (paramCast->paramID, &chain);
        }

        chain.listeners.call (&ProcessorChain::Listener::processorAdded, newProcPtr);
    }

    static void removeProcessor (ProcessorChain& chain, BaseProcessor* procToRemove, BaseProcessor::Ptr& saveProc)
    {
        Logger::writeToLog (String ("Removing processor: ") + procToRemove->getName());

        ProcessorChainHelpers::removeOutputConnectionsFromProcessor (chain, procToRemove, chain.um);

        chain.listeners.call (&ProcessorChain::Listener::processorRemoved, procToRemove);

        for (auto* param : procToRemove->getParameters())
        {
            if (auto* paramCast = dynamic_cast<juce::RangedAudioParameter*> (param))
                procToRemove->getVTS().removeParameterListener (paramCast->paramID, &chain);
        }

        chain.getActionHelper().actionQueue.emplace (
            [&procs = chain.procs, procToRemove, &saveProc]()
            {
                saveProc.reset (procs.removeAndReturn (procs.indexOf (procToRemove)));
            });
    }

    static void addConnection (ProcessorChain& chain, const ConnectionInfo& info)
    {
        Logger::writeToLog (String ("Adding connection from ") + info.startProc->getName() + ", port #"
                            + String (info.startPort) + " to " + info.endProc->getName() + " port #"
                            + String (info.endPort));

        info.startProc->addConnection (ConnectionInfo (info));
        chain.listeners.call (&ProcessorChain::Listener::connectionAdded, info);
    }

    static void removeConnection (ProcessorChain& chain, const ConnectionInfo& info)
    {
        Logger::writeToLog (String ("Removing connection from ") + info.startProc->getName() + ", port #"
                            + String (info.startPort) + " to " + info.endProc->getName() + " port #"
                            + String (info.endPort));

        info.startProc->removeConnection (info);
        chain.listeners.call (&ProcessorChain::Listener::connectionRemoved, info);
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

bool AddOrRemoveProcessor::perform()
{
    if (isRemoving)
    {
        jassert (actionProcPtr != nullptr);

        ProcChainActions::removeProcessor (chain, actionProcPtr, actionProc);
    }
    else
    {
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
        jassert (actionProc != nullptr);

        actionProcPtr = actionProc.get();
        ProcChainActions::addProcessor (chain, std::move (actionProc));
    }
    else
    {
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
