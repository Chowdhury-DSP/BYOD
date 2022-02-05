#include "PresetManager.h"
#include "PresetInfoHelpers.h"
#include "processors/chain/ProcessorChainStateHelper.h"

namespace
{
const String userPresetPath = "ChowdhuryDSP/BYOD/UserPresets.txt";
const String presetTag = "preset";
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

#if JUCE_IOS
    File appDataDir = File::getSpecialLocation (File::userApplicationDataDirectory);
    auto userPresetFolder = appDataDir.getChildFile (userPresetPath).getSiblingFile ("Presets");
    if (! userPresetFolder.isDirectory())
    {
        userPresetFolder.deleteFile();
        userPresetFolder.createDirectory();
    }

    setUserPresetPath (userPresetFolder);
#endif // JUCE_IOS
}

PresetManager::~PresetManager()
{
    userManager->removeListener (this);
}

void PresetManager::presetLoginStatusChanged()
{
    setUserPresetName (userManager->getUsername());
}

void PresetManager::syncLocalPresetsToServer()
{
    std::vector<PresetsServerSyncManager::AddedPresetInfo> addedPresetInfo;
    syncManager->syncLocalPresetsToServer (getUserPresets(), addedPresetInfo);

    // update preset ID's of newly added presets
    for (auto [constPreset, newPresetID] : addedPresetInfo)
    {
        for (auto& [_, preset] : presetMap)
        {
            if (preset == *constPreset)
            {
                PresetInfoHelpers::setPresetID (preset, newPresetID);
                preset.toFile (getPresetFile (preset));
                break;
            }
        }
    }
}

void PresetManager::syncServerPresetsToLocal (PresetUpdateList& presetsToUpdate)
{
    std::vector<chowdsp::Preset> serverPresets;
    syncManager->syncServerPresetsToLocal (serverPresets);

    // if an equivalent preset already exists, then we don't need to update it!
    const auto& userPresets = getUserPresets();
    serverPresets.erase (std::remove_if (serverPresets.begin(), serverPresets.end(), [&userPresets] (const auto& serverPreset)
                                         { return sst::cpputils::contains_if (userPresets, [&serverPreset] (const auto* userPreset)
                                                                              { return *userPreset == serverPreset; }); }),
                         serverPresets.end());

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

    procChain->getStateHelper().loadProcChain (xml, true);
}

File PresetManager::getPresetFile (const chowdsp::Preset& preset) const
{
    return getPresetFile (preset.getVendor(), preset.getCategory(), preset.getName());
}

File PresetManager::getPresetFile (const String& vendor, const String& category, const String& name) const
{
    return getUserPresetPath()
        .getChildFile (vendor)
        .getChildFile (category)
        .getChildFile (name + ".chowpreset");
}

void PresetManager::setUserPresetName (const String& newName)
{
    if (newName == userPresetsName)
        return;

    auto actualNewName = newName.isEmpty() ? "User" : newName;

    if (userIDMap.find (userPresetsName) != userIDMap.end()) // previous user name was the default!
    {
        // move existing user presets to new username
        int presetID = userIDMap[userPresetsName];
        while (presetMap.find (presetID) != presetMap.end())
        {
            auto& preset = presetMap.at (presetID++);

            const auto prevPresetFile = preset.getPresetFile();
            if (prevPresetFile != File())
                prevPresetFile.deleteFile();

            preset.setVendor (newName);
            preset.toFile (getPresetFile (preset));
        }
    }

    // delete existing presets with this username from the preset map
    for (auto presetMapIter = presetMap.begin(); presetMapIter != presetMap.end();)
    {
        if (presetMapIter->second.getVendor() == actualNewName)
            presetMapIter = presetMap.erase (presetMapIter);
        else
            ++presetMapIter;
    }

    userIDMap[actualNewName] = userUserIDStart;
    userIDMap.erase (userPresetsName);

    getUserPresetPath().getChildFile (userPresetsName).deleteRecursively();
    userPresetsName = actualNewName;

    loadUserPresetsFromFolder (getUserPresetPath());
}

void PresetManager::saveUserPreset (const String& name, const String& category, bool isPublic, const String& presetID)
{
    Logger::writeToLog ("Saving user preset, name: \"" + name + "\", category: \"" + category + "\"");

    auto stateXml = savePresetState();
    keepAlivePreset = std::make_unique<chowdsp::Preset> (name, getUserPresetName(), *stateXml, category);
    if (keepAlivePreset != nullptr)
    {
        PresetInfoHelpers::setIsPublic (*keepAlivePreset, isPublic);
        if (presetID.isNotEmpty())
            PresetInfoHelpers::setPresetID (*keepAlivePreset, presetID);

        keepAlivePreset->toFile (getPresetFile (*keepAlivePreset));
        loadPreset (*keepAlivePreset);

        loadUserPresetsFromFolder (getUserPresetPath());
    }
}
