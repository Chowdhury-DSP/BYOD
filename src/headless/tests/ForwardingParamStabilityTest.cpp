#include "UnitTests.h"
#include "processors/chain/ProcessorChainActionHelper.h"
#include "state/ParamForwardManager.h"

#include "processors/drive/BlondeDrive.h"
#include "processors/drive/RangeBooster.h"
#include "processors/drive/Warp.h"
#include "processors/drive/big_muff/BigMuffDrive.h"
#include "processors/drive/diode_circuits/DiodeClipper.h"
#include "processors/drive/diode_circuits/DiodeRectifier.h"
#include "processors/drive/flapjack/Flapjack.h"
#include "processors/drive/zen_drive/ZenDrive.h"
#include "processors/modulation/Chorus.h"
#include "processors/modulation/Rotary.h"
#include "processors/modulation/Tremolo.h"
#include "processors/modulation/phaser/Phaser4.h"
#include "processors/modulation/phaser/Phaser8.h"
#include "processors/modulation/scanner_vibrato/ScannerVibrato.h"
#include "processors/modulation/uni_vibe/UniVibe.h"
#include "processors/other/Compressor.h"
#include "processors/other/Delay.h"
#include "processors/other/EnvelopeFilter.h"
#include "processors/other/spring_reverb/SpringReverbProcessor.h"
#include "processors/tone/BassCleaner.h"
#include "processors/tone/BigMuffTone.h"
#include "processors/tone/BlondeTone.h"
#include "processors/tone/GraphicEQ.h"
#include "processors/tone/HighCut.h"
#include "processors/tone/LofiIrs.h"
#include "processors/tone/ladder_filter/LadderFilterProcessor.h"
#include "processors/tone/tube_screamer_tone/TubeScreamerTone.h"

template <typename ProcType>
static std::unique_ptr<BaseProcessor> processorFactory (UndoManager* um)
{
    return std::make_unique<ProcType> (um);
}

ProcessorStore::StoreMap minimalStore = {
    { "Blonde Drive", { &processorFactory<BlondeDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Diode Clipper", { &processorFactory<DiodeClipper>, { ProcessorType::Drive, 1, 1 } } },
    { "Diode Rectifier", { &processorFactory<DiodeRectifier>, { ProcessorType::Drive, 1, 1 } } },
    { "Flapjack", { &processorFactory<Flapjack>, { ProcessorType::Drive, 1, 1 } } },
    { "Muff Drive", { &processorFactory<BigMuffDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Range Booster", { &processorFactory<RangeBooster>, { ProcessorType::Drive, 1, 1 } } },
    { "Warp", { &processorFactory<Warp>, { ProcessorType::Drive, 1, 1 } } },
    { "Yen Drive", { &processorFactory<ZenDrive>, { ProcessorType::Drive, 1, 1 } } },
    { "Bass Cleaner", { &processorFactory<BassCleaner>, { ProcessorType::Tone, 1, 1 } } },
    { "Blonde Tone", { &processorFactory<BlondeTone>, { ProcessorType::Tone, 1, 1 } } },
    { "Graphic EQ", { &processorFactory<GraphicEQ>, { ProcessorType::Tone, 1, 1 } } },
    { "High Cut", { &processorFactory<HighCut>, { ProcessorType::Tone, 1, 1 } } },
    { "LoFi IRs", { &processorFactory<LofiIrs>, { ProcessorType::Tone, 1, 1 } } },
    { "Muff Tone", { &processorFactory<BigMuffTone>, { ProcessorType::Tone, 1, 1 } } },
    { "TS-Tone", { &processorFactory<TubeScreamerTone>, { ProcessorType::Tone, 1, 1 } } },
    { "Ladder Filter", { &processorFactory<LadderFilterProcessor>, { ProcessorType::Tone, 1, 1 } } },
    { "Chorus", { &processorFactory<Chorus>, { ProcessorType::Modulation, Chorus::numInputs, Chorus::numOutputs } } },
    { "Phaser4", { &processorFactory<Phaser4>, { ProcessorType::Modulation, Phaser4::numInputs, Phaser4::numOutputs } } },
    { "Phaser8", { &processorFactory<Phaser8>, { ProcessorType::Modulation, Phaser8::numInputs, Phaser8::numOutputs } } },
    { "Rotary", { &processorFactory<Rotary>, { ProcessorType::Modulation, Rotary::numInputs, Rotary::numOutputs } } },
    { "Scanner Vibrato", { &processorFactory<ScannerVibrato>, { ProcessorType::Modulation, ScannerVibrato::numInputs, ScannerVibrato::numOutputs } } },
    { "Solo-Vibe", { &processorFactory<UniVibe>, { ProcessorType::Modulation, UniVibe::numInputs, UniVibe::numOutputs } } },
    { "Tremolo", { &processorFactory<Tremolo>, { ProcessorType::Modulation, Tremolo::numInputs, Tremolo::numOutputs } } },
    { "DC Blocker", { &processorFactory<DCBlocker>, { ProcessorType::Utility, 1, 1 } } },
    { "Compressor", { &processorFactory<Compressor>, { ProcessorType::Other, Compressor::numInputs, Compressor::numOutputs } } },
    { "Delay", { &processorFactory<DelayModule>, { ProcessorType::Other, 1, 1 } } },
    { "Envelope Filter", { &processorFactory<EnvelopeFilter>, { ProcessorType::Other, EnvelopeFilter::numInputs, EnvelopeFilter::numOutputs } } },
};

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
        auto& storeMap = minimalStore;
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
        for (int count = 0; count < 19;)
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
            {
                expectEquals (forwardedParam->getName (1024).toStdString(),
                              paramNames[idx],
                              "Parameter name mismatch");
            }
            else
            {
                expectEquals (std::string {},
                              paramNames[idx],
                              "Parameter name mismatch");
            }
        }
    }

    void testLegacyState (int i)
    {
        /*
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
        };

        for (int count = 0; count < 9;)
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
        juce::StringArray paramNames {};
        for (auto [idx, param] : chowdsp::enumerate (forwardingParams))
        {
            if (auto* forwardedParam = param->getParam())
                paramNames.add (forwardedParam->getName (1024).toStdString());
            else
                paramNames.add ("EMPTY");
        }

        juce::MemoryBlock state;
        plugin.getStateInformation (state);

        const auto file = juce::File { "test_" + juce::String { i } };
        file.withFileExtension ("txt").replaceWithText (paramNames.joinIntoString (","));

        plugin.getXmlFromBinary (state.getData(), state.getSize())->writeToFile (file.withFileExtension ("xml"), "");
        */

        const auto [stateFile, paramsFile] = [i]
        {
            auto rootDir = File::getSpecialLocation (File::currentExecutableFile);
            while (rootDir.getFileName() != "BYOD")
                rootDir = rootDir.getParentDirectory();
            const auto fileDir = rootDir.getChildFile ("src/headless/tests/OldParamForwardStates");
            return std::make_pair (fileDir.getChildFile ("test_" + juce::String { i } + ".xml"),
                                   fileDir.getChildFile ("test_" + juce::String { i } + ".txt"));
        }();

        const auto stateXml = XmlDocument::parse (stateFile);
        BYOD plugin;
        juce::MemoryBlock state;
        plugin.copyXmlToBinary (*stateXml, state);
        plugin.setStateInformation (state.getData(), state.getSize());

        juce::StringArray paramNames;
        paramNames.addTokens (paramsFile.loadFileAsString(), ",", {});
        jassert (paramNames.size() == 500);
        for (auto& name : paramNames)
            name = name.trimCharactersAtStart (" ");

        auto& forwardingParams = plugin.getParamForwarder().getForwardedParameters();
        for (auto [idx, param] : chowdsp::enumerate (forwardingParams))
        {
            if (auto* forwardedParam = param->getParam())
            {
                expectEquals (forwardedParam->getName (1024),
                              paramNames[idx],
                              "Parameter name mismatch");
            }
            else
            {
                expectEquals (juce::String { "EMPTY" },
                              paramNames[idx],
                              "Parameter name mismatch");
            }
        }

        for (int i = 0; i < 20; ++i)
            removeProcessor (plugin.getProcChain());
        jassert (plugin.getProcChain().getProcessors().size() == 0);

        addProcessor (plugin.getProcChain(), &plugin.getUndoManager());
        auto* ip = reinterpret_cast<juce::RangedAudioParameter*> (plugin.getProcChain().getProcessors()[0]->getParameters()[0]);

        const auto& paramForwarder = plugin.getParamForwarder();
        expect (paramForwarder.getForwardedParameterFromInternal (*ip) == paramForwarder.getForwardedParameters()[0],
                "Newly added parameter is not in the first parameter slot!");
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

        beginTest ("Legacy Forwarding Parameter Compatibility Test");
        for (int i = 0; i < 5; ++i)
        {
            testLegacyState (i);
        }
    }

    Random rand;
};

static ForwardingParamStabilityTest forwardingParamStabilityTest;
