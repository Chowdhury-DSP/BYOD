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
    setUserPresetConfigFile (userPresetPath);
    setDefaultPreset (chowdsp::Preset { BinaryData::Default_chowpreset, BinaryData::Default_chowpresetSize });

    std::vector<chowdsp::Preset> factoryPresets;
    factoryPresets.emplace_back (BinaryData::ZenDrive_chowpreset, BinaryData::ZenDrive_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Centaur_chowpreset, BinaryData::Centaur_chowpresetSize);
    addPresets (factoryPresets);

    loadDefaultPreset();
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
