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

    loadFactoryPresets();
    setUserPresetConfigFile (userPresetPath);

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

void PresetManager::loadFactoryPresets()
{
    setDefaultPreset (chowdsp::Preset { BinaryData::Default_chowpreset, BinaryData::Default_chowpresetSize });

    std::vector<chowdsp::Preset> factoryPresets;
    // pedals
    factoryPresets.emplace_back (BinaryData::Big_Muff_chowpreset, BinaryData::Big_Muff_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Centaur_chowpreset, BinaryData::Centaur_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Instant_Metal_chowpreset, BinaryData::Instant_Metal_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::MXR_Distortion_chowpreset, BinaryData::MXR_Distortion_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Tube_Screamer_chowpreset, BinaryData::Tube_Screamer_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::ZenDrive_chowpreset, BinaryData::ZenDrive_chowpresetSize);

    // players
    factoryPresets.emplace_back (BinaryData::J_Mascis_chowpreset, BinaryData::J_Mascis_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::John_Mayer_chowpreset, BinaryData::John_Mayer_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Neil_Young_chowpreset, BinaryData::Neil_Young_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Nirvana_chowpreset, BinaryData::Nirvana_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::The_Strokes_chowpreset, BinaryData::The_Strokes_chowpresetSize);

    addPresets (factoryPresets);

    loadDefaultPreset();
}

void PresetManager::presetLoginStatusChanged()
{
    setUserPresetName (userManager->getUsername());
}

void PresetManager::syncLocalPresetsToServer()
{
    alertWindow.reset (LookAndFeel::getDefaultLookAndFeel().createAlertWindow ("Syncing Local Presets:", {}, {}, {}, {}, MessageBoxIconType::NoIcon, 0, nullptr));
    alertWindow->setEscapeKeyCancels (false);
    alertWindow->addProgressBarComponent (jobProgress);
    alertWindow->enterModalState();

    jobPool->addJob (
        [&]
        {
            auto updateJobProgress = [&] (int index, int numPresets)
            { jobProgress = double (index + 1) / (double) numPresets; };

            auto startNewJobSection = [&] (const String& message)
            {
                MessageManager::callAsync ([&, message]
                                           { alertWindow->setMessage (message); });
                jobProgress = 0.0;
            };

            startNewJobSection ("Syncing user presets...");
            std::vector<PresetsServerSyncManager::AddedPresetInfo> addedPresetInfo;
            syncManager->syncLocalPresetsToServer (getUserPresets(), addedPresetInfo, updateJobProgress);

            if (! addedPresetInfo.empty()) // no presets need new presetIDs!
            {
                startNewJobSection ("Updating preset IDs...");

                // update preset IDs of newly added presets
                std::vector<const chowdsp::Preset*> presetsNeedingPrsetIDUpdate;
                int index = 0;
                for (const auto& [constPreset, newPresetID] : addedPresetInfo)
                {
                    for (auto& [_, preset] : presetMap)
                    {
                        if (preset == *constPreset)
                        {
                            PresetInfoHelpers::setPresetID (preset, newPresetID);
                            presetsNeedingPrsetIDUpdate.push_back (&preset);
                            preset.toFile (getPresetFile (preset));
                            updateJobProgress (index++, (int) addedPresetInfo.size());
                            break;
                        }
                    }
                }

                // sync new preset IDs to server
                startNewJobSection ("Syncing new preset IDs...");
                addedPresetInfo.clear();
                syncManager->syncLocalPresetsToServer (presetsNeedingPrsetIDUpdate, addedPresetInfo, updateJobProgress);
                jassert (addedPresetInfo.empty());
            }

            MessageManager::callAsync (
                [&]
                {
                    alertWindow->exitModalState (1);
                    alertWindow.reset();
                });
        });
}

bool PresetManager::syncServerPresetsToLocal()
{
    serverSyncUpdatePresetsList.clear();
    std::vector<chowdsp::Preset> serverPresets;
    if (! syncManager->syncServerPresetsToLocal (serverPresets))
        return false;

    // if an equivalent preset already exists, then we don't need to update it!
    const auto& userPresets = getUserPresets();
    serverPresets.erase (std::remove_if (serverPresets.begin(), serverPresets.end(), [&userPresets] (const auto& serverPreset)
                                         { return sst::cpputils::contains_if (userPresets, [&serverPreset] (const auto* userPreset)
                                                                              { return *userPreset == serverPreset; }); }),
                         serverPresets.end());

    // mark if presets are being added or overwritten
    serverSyncUpdatePresetsList.reserve (serverPresets.size());
    for (auto& preset : serverPresets)
    {
        auto updateType = sst::cpputils::contains_if (userPresets, [&preset] (const auto* userPreset)
                                                      { return userPreset->getName() == preset.getName(); })
                              ? PresetUpdate::Overwriting
                              : PresetUpdate::Adding;
        serverSyncUpdatePresetsList.emplace_back (std::move (preset), updateType);
    }

    return true;
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
        .getChildFile (name + PresetConstants::presetExt);
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
    sst::cpputils::nodal_erase_if (presetMap, [actualNewName] (const auto& presetPair)
                                   { return presetPair.second.getVendor() == actualNewName; });

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

void PresetManager::loadUserPresetsFromFolder (const juce::File& file)
{
    std::vector<chowdsp::Preset> presets;
    for (const auto& f : file.findChildFiles (juce::File::findFiles, true, "*" + PresetConstants::presetExt))
        presets.push_back (loadUserPresetFromFile (f));

    // delete old user presets
    sst::cpputils::nodal_erase_if (presetMap, [] (const auto& presetPair)
                                   { return presetPair.second.getVendor() != PresetConstants::factoryPresetVendor; });

    int presetID = userIDMap[userPresetsName];
    while (presetMap.find (presetID) != presetMap.end())
        presetMap.erase (presetID++);

    addPresets (presets);
}
