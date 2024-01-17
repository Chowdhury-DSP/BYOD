#include "UnitTests.h"
#include "processors/chain/ProcessorChainActionHelper.h"
#include "state/ParamForwardManager.h"

class ForwardingParamStabilityTest : public UnitTest
{
public:
    ForwardingParamStabilityTest()
        : UnitTest ("Forwarding Parameter Stability Test")
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
        actionHelper.addProcessor (storeIter->second.factory (um));

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

    using ParamNames = std::array<std::string, 500>;
    auto runPlugin()
    {
        BYOD plugin;
        auto* undoManager = plugin.getVTS().undoManager;
        auto& procChain = plugin.getProcChain();

        struct Action
        {
            String name;
            std::function<bool()> action;
        };

        std::vector<Action> actions {
            { "Add Processor", [&]
              { return addProcessor (procChain, undoManager); } },
            { "Add Processor", [&]
              { return addProcessor (procChain, undoManager); } },
            { "Remove Processor", [&]
              { return removeProcessor (procChain); } },
        };

#if JUCE_DEBUG
        for (int count = 0; count < 9;)
#else
        for (int count = 0; count < 101;)
#endif
        {
            auto& action = actions[rand.nextInt ((int) actions.size())];
            if (action.action())
            {
                int timeUntilNextAction = rand.nextInt ({ 5, 500 });
                juce::MessageManager::getInstance()->runDispatchLoopUntil (timeUntilNextAction);
                count++;
            }
        }

        auto& forwardingParams = plugin.getParamForwarder().getForwardedParameters();
        ParamNames paramNames {};
        for (auto [idx, param] : chowdsp::enumerate (forwardingParams))
        {
            if (auto* forwardedParam = param->getParam())
                paramNames[idx] = forwardedParam->getName (1024).toStdString();
        }

        juce::MemoryBlock state;
        plugin.getStateInformation (state);

        return std::make_tuple (paramNames, state);
    }

    void testPlugin (const ParamNames& paramNames, const juce::MemoryBlock& state)
    {
        BYOD plugin;
        plugin.setStateInformation (state.getData(), (int) state.getSize());

        auto& forwardingParams = plugin.getParamForwarder().getForwardedParameters();
        for (auto [idx, param] : chowdsp::enumerate (forwardingParams))
        {
            if (auto* forwardedParam = param->getParam())
                expectEquals (forwardedParam->getName (1024).toStdString(),
                              paramNames[idx],
                              "Parameter name mismatch");
            else
                expectEquals (std::string {},
                              paramNames[idx],
                              "Parameter name mismatch");
        }
    }

    void runTest() override
    {
        beginTest ("Check Max Parameter Count");
        runTestForAllProcessors (
            this,
            [this] (BaseProcessor* proc)
            {
                expectLessThan (proc->getParameters().size(),
                                ParamForwardManager::maxParameterCount,
                                "");
            },
            {},
            false);

        beginTest ("Forwarding Parameter Stability Test");
        rand = Random { 1245 };
#if JUCE_DEBUG
        for (int i = 0; i < 1; ++i)
#else
        for (int i = 0; i < 10; ++i)
#endif
        {
            const auto [paramNames, state] = runPlugin();
            testPlugin (paramNames, state);
        }
    }

    Random rand;
};

static ForwardingParamStabilityTest forwardingParamStabilityTest;
