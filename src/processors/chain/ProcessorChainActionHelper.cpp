#include "ProcessorChainActionHelper.h"
#include "ProcessorChainActions.h"

ProcessorChainActionHelper::ProcessorChainActionHelper (ProcessorChain& thisChain) : chain (thisChain),
                                                                                     um (chain.um)
{
    chain.procStore.addProcessorCallback = [=] (auto newProc) { addProcessor (std::move (newProc)); };
    chain.procStore.replaceProcessorCallback = [=] (auto newProc, auto procToReplace) { replaceProcessor (std::move (newProc), procToReplace); };
}

void ProcessorChainActionHelper::addProcessor (BaseProcessor::Ptr newProc)
{
    um->beginNewTransaction();
    um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));
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
    // 1-to-1 replacement requires the same I/O channels!
    jassert (newProc->getNumInputs() == procToReplace->getNumInputs());
    jassert (newProc->getNumOutputs() == procToReplace->getNumOutputs());

    um->beginNewTransaction();

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
                    newConnection.endProc = newProc.get();

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
            newConnection.startProc = newProc.get();

            um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
            um->perform (new AddOrRemoveConnection (chain, std::move (newConnection)));
        }
    }

    newProc->setPosition (*procToReplace);

    um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));
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
