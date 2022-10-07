#include "ProcessorChainActionHelper.h"
#include "ProcessorChainActions.h"

ProcessorChainActionHelper::ProcessorChainActionHelper (ProcessorChain& thisChain) : chain (thisChain),
                                                                                     um (chain.um)
{
    chain.procStore.addProcessorCallback = [=] (auto newProc)
    { addProcessor (std::move (newProc)); };
    chain.procStore.replaceProcessorCallback = [=] (auto newProc, auto procToReplace)
    { replaceProcessor (std::move (newProc), procToReplace); };
    chain.procStore.replaceConnectionWithProcessorCallback = [=] (auto newProc, auto connectionInfo)
    { replaceConnectionWithProcessor (std::move (newProc), connectionInfo); };
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

void ProcessorChainActionHelper::replaceConnectionWithProcessor (BaseProcessor::Ptr newProc, ConnectionInfo& connectionInfo)
{
    um->beginNewTransaction();

    um->perform (new AddOrRemoveConnection (chain, { connectionInfo.startProc, connectionInfo.startPort, connectionInfo.endProc, connectionInfo.endPort }, true));

    auto newProcRaw = newProc.get();
    um->perform (new AddOrRemoveProcessor (chain, std::move (newProc)));

    um->perform (new AddOrRemoveConnection (chain, { connectionInfo.startProc, connectionInfo.startPort, newProcRaw, 0 }));
    um->perform (new AddOrRemoveConnection (chain, { newProcRaw, 0, connectionInfo.endProc, connectionInfo.endPort }));
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
        // only 1-in/1-out modules can be replaced with nothing
        jassert (procToReplace->getNumInputs() == 1);
        jassert (procToReplace->getNumOutputs() == 1);

        um->beginNewTransaction();

        const auto removeIncomingConnection = [this, &procToReplace] (BaseProcessor* proc) -> std::pair<BaseProcessor*, int>
        {
            for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
            {
                int numConnections = proc->getNumOutputConnections (portIdx);
                for (int cIdx = numConnections - 1; cIdx >= 0; --cIdx)
                {
                    auto connection = proc->getOutputConnection (portIdx, cIdx);
                    if (connection.endProc == procToReplace)
                    {
                        auto* startProc = connection.startProc;
                        const auto startPort = connection.startPort;
                        um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));
                        return std::make_pair (startProc, startPort);
                    }
                }
            }
            return {};
        };

        auto [startProc, startPort] = removeIncomingConnection (&chain.inputProcessor);
        for (auto* proc : chain.procs)
        {
            if (startProc != nullptr)
                break;

            if (proc == procToReplace)
                continue;

            std::tie (startProc, startPort) = removeIncomingConnection (proc);
        }

        // swap all outgoing connections from proc to replace
        for (int portIdx = 0; portIdx < procToReplace->getNumOutputs(); ++portIdx)
        {
            int numConnections = procToReplace->getNumOutputConnections (portIdx);
            for (int cIdx = numConnections - 1; cIdx >= 0; --cIdx)
            {
                auto connection = procToReplace->getOutputConnection (portIdx, cIdx);
                auto newConnection = connection;
                newConnection.startProc = startProc;
                newConnection.startPort = startPort;

                um->perform (new AddOrRemoveConnection (chain, std::move (connection), true));

                if (startProc != nullptr)
                    um->perform (new AddOrRemoveConnection (chain, std::move (newConnection)));
            }
        }

        um->perform (new AddOrRemoveProcessor (chain, procToReplace));

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
