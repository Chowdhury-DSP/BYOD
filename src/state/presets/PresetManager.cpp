#include "PresetManager.h"
#include "processors/chain/ProcessorChainStateHelper.h"

namespace
{
String userPresetPath = "ChowdhuryDSP/BYOD/UserPresets.txt";
String presetTag = "preset";
} // namespace

PresetManager::PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vtState) : chowdsp::PresetManager (vtState),
                                                                                              procChain (chain)
{
    if (userManager->getUsername().isNotEmpty())
        setUserPresetName (userManager->getUsername());

    userManager->addListener (this);

    setUserPresetConfigFile (userPresetPath);
    setDefaultPreset (chowdsp::Preset { BinaryData::Default_chowpreset, BinaryData::Default_chowpresetSize });

    std::vector<chowdsp::Preset> factoryPresets;
    factoryPresets.emplace_back (BinaryData::ZenDrive_chowpreset, BinaryData::ZenDrive_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Centaur_chowpreset, BinaryData::Centaur_chowpresetSize);
    addPresets (factoryPresets);

    loadDefaultPreset();
}

PresetManager::~PresetManager()
{
    userManager->removeListener (this);
}

void PresetManager::presetLoginStatusChanged()
{
    setUserPresetName (userManager->getUsername());
}

void PresetManager::syncLocalPresetsToServer() const
{
    syncManager->syncLocalPresetsToServer (getUserPresets());
}

void PresetManager::syncServerPresetsToLocal (std::vector<chowdsp::Preset>& presetsToUpdate)
{
    syncManager->syncServerPresetsToLocal (presetsToUpdate);

    // if an equivalent preset already exists, then we don't need to update it!
    const auto& userPresets = getUserPresets();
    std::erase_if (presetsToUpdate, [&userPresets] (const auto& serverPreset)
                   { return sst::cpputils::contains_if (userPresets, [&serverPreset] (const auto* userPreset)
                                                        { return *userPreset == serverPreset; }); });
}

std::unique_ptr<XmlElement> PresetManager::savePresetState()
{
    return procChain->getStateHelper().saveProcChain();
}

void PresetManager::loadPresetState (const XmlElement* xml)
{
    Logger::writeToLog ("Loading preset: " + getCurrentPreset()->getName());

    procChain->getStateHelper().loadProcChain (xml);
}
