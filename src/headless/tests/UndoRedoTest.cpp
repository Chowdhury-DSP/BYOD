#include "UnitTests.h"
#include "processors/chain/ProcessorChainActionHelper.h"

class UndoRedoTest : public UnitTest
{
public:
    UndoRedoTest() : UnitTest ("Undo/Redo Test")
    {
    }

    bool addProcessor (ProcessorChain& chain, UndoManager* um)
    {
        // get random processor
        auto& storeMap = ProcessorStore::getStoreMap();
        auto storeIter = storeMap.begin();
        int storeIndex = rand.nextInt ((int) storeMap.size());
        std::advance (storeIter, storeIndex);

        auto& actionHelper = chain.getActionHelper();
        actionHelper.addProcessor (storeIter->second (um));

        return true;
    }

    bool removeProcessor (ProcessorChain& chain)
    {
        const auto& procs = chain.getProcessors();
        if (procs.isEmpty())
            return false;

        auto procIndexToRemove = rand.nextInt (procs.size());
        auto* procToRemove = procs[procIndexToRemove];

        auto& actionHelper = chain.getActionHelper();
        actionHelper.removeProcessor (procToRemove);

        return true;
    }

    bool addConnection (ProcessorChain& chain)
    {
        const auto& procs = chain.getProcessors();
        if (procs.isEmpty())
            return false;

        auto startProcIndex = rand.nextInt (procs.size() + 1);
        auto* startProc = startProcIndex < procs.size() ? procs[startProcIndex] : &chain.getInputProcessor();
        if (startProc->getNumOutputs() < 1)
            return false;

        int startPortIndex = rand.nextInt (startProc->getNumOutputs());

        auto endProcIndex = rand.nextInt (procs.size() + 1);
        auto* endProc = endProcIndex < procs.size() ? procs[endProcIndex] : &chain.getOutputProcessor();

        auto isInputConnected = [&procs] (BaseProcessor* endProc) -> bool
        {
            for (const auto* proc : procs)
            {
                for (int i = 0; i < proc->getNumOutputs(); ++i)
                {
                    for (int j = 0; j < proc->getNumOutputConnections (i); ++j)
                    {
                        auto& connection = proc->getOutputConnection (i, j);
                        if (connection.endProc == endProc)
                            return true;
                    }
                }
            }

            return false;
        };

        int endPortIndex = 0;
        for (; endPortIndex < endProc->getNumInputs(); ++endPortIndex)
        {
            if (! isInputConnected (endProc))
                break;
        }

        if (endPortIndex == endProc->getNumInputs())
            return false;

        auto& actionHelper = chain.getActionHelper();
        actionHelper.addConnection ({ startProc, startPortIndex, endProc, endPortIndex });

        return true;
    }

    bool removeConnection (ProcessorChain& chain)
    {
        const auto& procs = chain.getProcessors();
        auto startProcIndex = rand.nextInt (procs.size() + 1);
        auto* startProc = startProcIndex < procs.size() ? procs[startProcIndex] : &chain.getInputProcessor();

        int startPortIndex = 0;
        for (; startPortIndex < startProc->getNumOutputs(); ++startPortIndex)
        {
            if (startProc->getNumOutputConnections (startPortIndex) > 0)
                break;
        }

        if (startPortIndex == startProc->getNumOutputs())
            return false;

        auto& actionHelper = chain.getActionHelper();
        auto connectionToRemove = startProc->getOutputConnection (startPortIndex, 0);
        actionHelper.removeConnection (std::move (connectionToRemove));

        return true;
    }

    void runTest() override
    {
        rand = getRandom();

        beginTest ("Undo/Redo Test");

        BYOD plugin;
        auto* undoManager = plugin.getVTS().undoManager;
        auto& procChain = plugin.getProcChain();

        struct Action
        {
            String name;
            std::function<bool()> action;
            int numTimesCalled = 0;
        };

        std::vector<Action> actions {
            { "Add Processor", [&]
              { return addProcessor (procChain, undoManager); } },
            { "Remove Processor", [&]
              { return removeProcessor (procChain); } },
            { "Add Connection", [&]
              { return addConnection (procChain); } },
            { "Remove Connection", [&]
              { return removeConnection (procChain); } },
            { "Undo", [&]
              { return undoManager->undo(); } },
            { "Redo", [&]
              { return undoManager->redo(); } }
        };

        constexpr int nIter = 100;
        auto rand = getRandom();
        for (int count = 0; count < nIter;)
        {
            auto& action = actions[rand.nextInt ((int) actions.size())];
            if (action.action())
            {
                int timeUntilNextAction = rand.nextInt ({ 5, 500 });
                juce::MessageManager::getInstance()->runDispatchLoopUntil (timeUntilNextAction);
                action.numTimesCalled++;
                count++;
            }
        }

        for (const auto& action : actions)
            std::cout << "Action: " << action.name << " succesfully called " << action.numTimesCalled << " times!" << std::endl;
    }

private:
    Random rand;
};

static UndoRedoTest undoRedoTest;
