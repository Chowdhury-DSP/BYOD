#include "PresetManager.h"
#include "processors/ProcessorChain.h"

namespace
{
static String userPresetPath = "ChowdhuryDSP/BYOD/UserPresets.txt";
static String presetTag = "preset";
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
    return procChain->saveProcChain();
}

void PresetManager::loadPresetState (const XmlElement* xml)
{
    MessageManager::callAsync ([=]
                               { procChain->loadProcChain (xml); });
}
