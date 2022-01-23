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

void PresetManager::syncServerPresetsToLocal (PresetUpdateList& presetsToUpdate)
{
    std::vector<chowdsp::Preset> serverPresets;
    syncManager->syncServerPresetsToLocal (serverPresets);

    // if an equivalent preset already exists, then we don't need to update it!
    const auto& userPresets = getUserPresets();
    std::erase_if (serverPresets, [&userPresets] (const auto& serverPreset)
                   { return sst::cpputils::contains_if (userPresets, [&serverPreset] (const auto* userPreset)
                                                        { return *userPreset == serverPreset; }); });

    // mark if presets are being added or overwritten
    presetsToUpdate.reserve (serverPresets.size());
    for (auto& preset : serverPresets)
    {
        auto updateType = sst::cpputils::contains_if (userPresets, [&preset] (const auto* userPreset)
                                                      { return userPreset->getName() == preset.getName(); })
                              ? PresetUpdate::Overwriting
                              : PresetUpdate::Adding;
        presetsToUpdate.emplace_back (std::move (preset), updateType);
    }
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
