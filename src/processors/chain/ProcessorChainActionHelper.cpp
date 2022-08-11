#include "ProcessorChainActionHelper.h"
#include "ProcessorChainActions.h"

ProcessorChainActionHelper::ProcessorChainActionHelper (ProcessorChain& thisChain) : chain (thisChain),
                                                                                     um (chain.um)
{
    chain.procStore.addProcessorCallback = [=] (auto newProc)
    { addProcessor (std::move (newProc)); };
    chain.procStore.replaceProcessorCallback = [=] (auto newProc, auto procToReplace)
    { replaceProcessor (std::move (newProc), procToReplace); };
<<<<<<< HEAD
=======
    chain.procStore.addProcessorFromCableClickCallback = [=] (auto newProc, auto startProc, auto endProc)
    { addProcessorFromCableClick (std::move (newProc), startProc, endProc); };

    startTimer (50);
>>>>>>> PR changes, improved cable click menu
}

ProcessorChainActionHelper::~ProcessorChainActionHelper() = default;

void ProcessorChainActionHelper::addProcessor (BaseProcessor::Ptr newProc)
{
    if (newProc == nullptr)
    {
        jassertfalse; // unable to create this processor!
        return;
    }
    else
    {
        um->beginNewTransaction();
        um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));
    }
}

void ProcessorChainActionHelper::addProcessorFromCableClick (BaseProcessor::Ptr newProc, BaseProcessor* startProc, BaseProcessor* endProc)
{
    um->beginNewTransaction();

    // Destroy old cable connection
    auto minIncomingPorts = jmin (endProc->getNumInputs(), startProc->getNumOutputs());
    for (int portIdx = 0; portIdx < minIncomingPorts; ++portIdx)
    {
        um->perform (new AddOrRemoveConnection (chain, { startProc, portIdx, endProc, portIdx }, true));
    }

    // In order for the undo manager to work properly, when it destroys the processor on an undo there should be no
    // output connections for that processor,otherwise we run into recursive undos. To mitigate this, we can add the
    // newProc first, then add the connections to it so when an undo happens, the connections are removed first, then
    // the processor is removed last. We get the newProcRaw values because after the addition of the newProc with
    // std:move(newProc) newProc will be null and we wont be able to use it for adding connection info.
    BaseProcessor* newProcRaw = newProc.get();
    um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));

    // Attach cable(s) to newProc input
    for (int portIdx = 0; portIdx < minIncomingPorts; ++portIdx)
    {
        um->perform (new AddOrRemoveConnection (chain, { startProc, portIdx, newProcRaw, portIdx }));
    }

    // Attach cable(s) to newProc output
    auto minOutgoingPorts = jmin (endProc->getNumInputs(), newProcRaw->getNumOutputs()); //bad name
    for (int portIdx = 0; portIdx < minOutgoingPorts; ++portIdx)
    {
        um->perform (new AddOrRemoveConnection (chain, { newProcRaw, portIdx, endProc, portIdx }));
    }
}

void ProcessorChainActionHelper::removeProcessor (BaseProcessor* procToRemove)
{
    um->beginNewTransaction();

    auto removeConnections = [=] (BaseProcessor* proc)
    {
        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            int numConnections = proc->getNumOutputConnections (portIdx);
            for (int cIdx = numConnections - 1; cIdx >= 0; --cIdx)
            {
                auto connection = proc->getOutputConnection (portIdx, cIdx);
                if (connection.endProc == procToRemove)
                    um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
            }
        }
    };

    for (auto* proc : chain.procs)
    {
        if (proc == procToRemove)
            continue;

        removeConnections (proc);
    }

    removeConnections (&chain.inputProcessor);

    um->perform (new AddOrRemoveProcessor (chain, procToRemove));
}

void ProcessorChainActionHelper::replaceProcessor (BaseProcessor::Ptr newProc, BaseProcessor* procToReplace)
{
    if (newProc == nullptr)
    {
        jassertfalse; // unable to create this processor!
        return;
    }

    // 1-to-1 replacement requires the same I/O channels!
    jassert (newProc->getNumInputs() == procToReplace->getNumInputs());
    jassert (newProc->getNumOutputs() == procToReplace->getNumOutputs());

    um->beginNewTransaction();

    newProc->setPosition (*procToReplace);
    um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));
    auto* procJustAdded = chain.procs.getLast();

    auto swapConnections = [&] (BaseProcessor* proc)
    {
        for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
        {
            int numConnections = proc->getNumOutputConnections (portIdx);
            for (int cIdx = numConnections - 1; cIdx >= 0; --cIdx)
            {
                auto connection = proc->getOutputConnection (portIdx, cIdx);
                if (connection.endProc == procToReplace)
                {
                    auto newConnection = connection;
                    newConnection.endProc = procJustAdded;

                    um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
                    um->perform (new AddOrRemoveConnection (chain, std::move (newConnection)));
                }
            }
        }
    };

    // swap all incoming connections to proc to replace
    for (auto* proc : chain.procs)
    {
        if (proc == procToReplace)
            continue;

        swapConnections (proc);
    }

    swapConnections (&chain.inputProcessor);

    // swap all outgoing connections from proc to replace
    for (int portIdx = 0; portIdx < procToReplace->getNumOutputs(); ++portIdx)
    {
        int numConnections = procToReplace->getNumOutputConnections (portIdx);
        for (int cIdx = numConnections - 1; cIdx >= 0; --cIdx)
        {
            auto connection = procToReplace->getOutputConnection (portIdx, cIdx);
            auto newConnection = connection;
            newConnection.startProc = procJustAdded;

            um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
            um->perform (new AddOrRemoveConnection (chain, std::move (newConnection)));
        }
    }

    um->perform (new AddOrRemoveProcessor (chain, procToReplace));
}


void ProcessorChainActionHelper::addConnection (ConnectionInfo&& info)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveConnection (chain, std::move (info)));
}

void ProcessorChainActionHelper::removeConnection (ConnectionInfo&& info)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveConnection (chain, std::move (info), true));
}
