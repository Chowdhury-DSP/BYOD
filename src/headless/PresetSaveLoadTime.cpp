#include "PresetSaveLoadTime.h"
#include "BYOD.h"
#include "processors/chain/ProcessorChainActionHelper.h"

constexpr int nProcs = 50;

PresetSaveLoadTime::PresetSaveLoadTime()
{
    this->commandOption = "--preset-save-load-time";
    this->argumentDescription = "--preset-save-load-time";
    this->shortDescription = "Times how long it takes to save and load a large preset";
    this->longDescription = "";
    this->command = [=] (const ArgumentList& args)
    { timePresetSaveAndLoad (args); };
}

void setUpInitialProcChain (ProcessorChain& procChain, ProcessorChainActionHelper& actionHelper, UndoManager* um)
{
    auto connectionToRemove = procChain.getInputProcessor().getOutputConnection (0, 0);
    actionHelper.removeConnection (std::move (connectionToRemove));

    BaseProcessor* prevProc = &procChain.getInputProcessor();
    auto& storeMap = ProcessorStore::getStoreMap();
    Random rand;

    for (int i = 0; i < nProcs; ++i)
    {
        auto storeIter = storeMap.begin();
        int storeIndex = rand.nextInt ((int) storeMap.size());
        std::advance (storeIter, storeIndex);

        actionHelper.addProcessor (storeIter->second.factory (um));
        auto* newProc = procChain.getProcessors().getLast();
        actionHelper.addConnection ({ prevProc, 0, newProc, 0 });

        prevProc = newProc;
    }

    actionHelper.addConnection ({ prevProc, 0, &procChain.getOutputProcessor(), 0 });
}

auto timeSavePreset (chowdsp::PresetManager& presetManager, const File& file)
{
    auto start = Time::getMillisecondCounterHiRes();
    presetManager.saveUserPreset (file);
    auto duration = (Time::getMillisecondCounterHiRes() - start) / 1000.0;

    return duration;
}

auto timeLoadDefaultPreset (chowdsp::PresetManager& presetManager)
{
    auto start = Time::getMillisecondCounterHiRes();
    presetManager.loadDefaultPreset();
    auto duration = (Time::getMillisecondCounterHiRes() - start) / 1000.0;

    return duration;
}

auto timeLoadPreset (chowdsp::PresetManager& presetManager, const File& file)
{
    auto start = Time::getMillisecondCounterHiRes();
    presetManager.loadPreset (chowdsp::Preset { file });
    auto duration = (Time::getMillisecondCounterHiRes() - start) / 1000.0;

    return duration;
}

void PresetSaveLoadTime::timePresetSaveAndLoad (const ArgumentList&)
{
    BYOD plugin;
    auto& presetManager = plugin.getPresetManager();
    auto& procChain = plugin.getProcChain();
    auto& actionHelper = procChain.getActionHelper();
    auto* undoManager = plugin.getVTS().undoManager;

    setUpInitialProcChain (procChain, actionHelper, undoManager);

    File presetFile = File::getSpecialLocation (File::userHomeDirectory).getChildFile ("test_preset_file");

    auto saveDuration = timeSavePreset (presetManager, presetFile);
    auto loadDefaultDuration = timeLoadDefaultPreset (presetManager);
    auto loadDuration = timeLoadPreset (presetManager, presetFile);

    std::cout << "Saved large preset in " << saveDuration << " seconds" << std::endl;
    std::cout << "Loaded default preset in " << loadDefaultDuration << " seconds" << std::endl;
    std::cout << "Loaded large preset in " << loadDuration << " seconds" << std::endl;

    presetFile.deleteFile();
}
