#include "PresetManager.h"
#include "../StateManager.h"
#include "PresetInfoHelpers.h"
#include "gui/utils/ErrorMessageView.h"
#include "processors/chain/ProcessorChainStateHelper.h"

#if BYOD_ENABLE_ADD_ON_MODULES
#include <AddOnPresets.h>
#endif

class ChangePresetAction : public UndoableAction
{
public:
    explicit ChangePresetAction (PresetManager& mgr) : manager (mgr) {}

    bool perform() override
    {
        if (keepAlivePreset != nullptr)
        {
            std::swap (manager.currentPreset, keepAlivePreset);

            const auto swapDirty = manager.getIsDirty();
            manager.setIsDirty (presetWasDirty);
            presetWasDirty = swapDirty;

            manager.listeners.call (&chowdsp::PresetManager::Listener::selectedPresetChanged);
            return true;
        }

        keepAlivePreset = manager.getCurrentPreset();
        presetWasDirty = manager.getIsDirty();
        return keepAlivePreset != nullptr;
    }

    bool undo() override { return perform(); }
    int getSizeInUnits() override { return 500; } // allow for 100 preset change actions

private:
    PresetManager& manager;
    const chowdsp::Preset* keepAlivePreset = nullptr;
    bool presetWasDirty = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChangePresetAction)
};

void PresetManager::showErrorMessage (const String& title, const String& message, Component* associatedComp)
{
    ErrorMessageView::showErrorMessage (title, message, "OK", associatedComp);
}

PresetManager::PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vtState)
    : chowdsp::PresetManager (vtState),
      procChain (chain)
{
#if BYOD_BUILD_PRESET_SERVER
    if (userManager->getUsername().isNotEmpty())
        setUserPresetName (userManager->getUsername());

    userManager->addListener (this);
#endif

    auto factoryPresets = getFactoryPresets (chain->getProcStore());
    addPresets (factoryPresets);

    setDefaultPreset (chowdsp::Preset { BinaryData::Default_chowpreset, BinaryData::Default_chowpresetSize });
    loadDefaultPreset();
    vts.undoManager->clearUndoHistory();

    setUserPresetConfigFile (chowdsp::toString (userPresetPath));

#if JUCE_IOS
    const auto groupDir = File::getContainerForSecurityApplicationGroupIdentifier ("group.com.chowdsp.BYOD");
    if (groupDir != File())
    {
        auto userPresetFolder = groupDir.getChildFile (chowdsp::toString (userPresetPath)).getSiblingFile ("Presets");
        if (! userPresetFolder.isDirectory())
        {
            userPresetFolder.deleteFile();
            userPresetFolder.createDirectory();
        }

        setUserPresetPath (userPresetFolder);
    }
#endif // JUCE_IOS
}

PresetManager::~PresetManager() // NOLINT
{
#if BYOD_BUILD_PRESET_SERVER
    userManager->removeListener (this);
#endif
}

#if BYOD_BUILD_PRESET_SERVER
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
#endif // BYOD_BUILD_PRESET_SERVER

static void sortPresets (std::vector<chowdsp::Preset>& presets)
{
    std::sort (presets.begin(),
               presets.end(),
               [] (const chowdsp::Preset& p1, const chowdsp::Preset& p2)
               {
                   if (p1.getVendor() != p2.getVendor())
                       return p1.getVendor() < p2.getVendor();

                   if (p1.getCategory() != p2.getCategory())
                       return p1.getCategory() < p2.getCategory();

                   if (p1.getCategory().isEmpty())
                       return p1.getName() > p2.getName();

                   return p1.getName() < p2.getName();
               });
}

std::vector<chowdsp::Preset> PresetManager::getFactoryPresets (const ProcessorStore& procStore)
{
    std::vector<chowdsp::Preset> factoryPresets;

    // default
    factoryPresets.emplace_back (BinaryData::Default_chowpreset, BinaryData::Default_chowpresetSize);

    // amps
    factoryPresets.emplace_back (BinaryData::Instant_Metal_chowpreset, BinaryData::Instant_Metal_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Bass_Face_chowpreset, BinaryData::Bass_Face_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Modern_HiGain_chowpreset, BinaryData::Modern_HiGain_chowpresetSize);

    // modulation
    factoryPresets.emplace_back (BinaryData::Chopped_Flange_chowpreset, BinaryData::Chopped_Flange_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Mixed_In_Modulation_chowpreset, BinaryData::Mixed_In_Modulation_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Seasick_Phase_chowpreset, BinaryData::Seasick_Phase_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Laser_Cave_chowpreset, BinaryData::Laser_Cave_chowpresetSize);

    // pedals
    factoryPresets.emplace_back (BinaryData::American_Sound_chowpreset, BinaryData::American_Sound_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Big_Muff_chowpreset, BinaryData::Big_Muff_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Big_Muff_Triangle_chowpreset, BinaryData::Big_Muff_Triangle_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Big_Muff_Rams_Head_56_chowpreset, BinaryData::Big_Muff_Rams_Head_56_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Big_Muff_Russian_chowpreset, BinaryData::Big_Muff_Russian_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Centaur_chowpreset, BinaryData::Centaur_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Gainful_Clipper_chowpreset, BinaryData::Gainful_Clipper_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Hot_Cakes_chowpreset, BinaryData::Hot_Cakes_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Hot_Fuzz_chowpreset, BinaryData::Hot_Fuzz_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::King_Of_Tone_chowpreset, BinaryData::King_Of_Tone_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::MXR_Distortion_chowpreset, BinaryData::MXR_Distortion_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::RAT_chowpreset, BinaryData::RAT_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Tube_Screamer_chowpreset, BinaryData::Tube_Screamer_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Violet_Mist_chowpreset, BinaryData::Violet_Mist_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Wah_Pedal_chowpreset, BinaryData::Wah_Pedal_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::ZenDrive_chowpreset, BinaryData::ZenDrive_chowpresetSize);

    // players
    factoryPresets.emplace_back (BinaryData::Black_Sabbath_chowpreset, BinaryData::Black_Sabbath_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Boston_chowpreset, BinaryData::Boston_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Clapton_chowpreset, BinaryData::Clapton_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::George_Harrison_chowpreset, BinaryData::George_Harrison_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Green_Day_chowpreset, BinaryData::Green_Day_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::J_Mascis_chowpreset, BinaryData::J_Mascis_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Jimi_Hendrix_chowpreset, BinaryData::Jimi_Hendrix_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::John_Mayer_chowpreset, BinaryData::John_Mayer_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Johnny_Greenwood_chowpreset, BinaryData::Johnny_Greenwood_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Neil_Young_chowpreset, BinaryData::Neil_Young_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Nirvana_chowpreset, BinaryData::Nirvana_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Pete_Townshend_chowpreset, BinaryData::Pete_Townshend_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::Superdrag_chowpreset, BinaryData::Superdrag_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::The_Strokes_chowpreset, BinaryData::The_Strokes_chowpresetSize);
    factoryPresets.emplace_back (BinaryData::White_Stripes_chowpreset, BinaryData::White_Stripes_chowpresetSize);

#if BYOD_ENABLE_ADD_ON_MODULES
    AddOnPresets::addFactoryPresets (factoryPresets);
#endif

    filterPresets (factoryPresets, procStore);
    sortPresets (factoryPresets);

    return factoryPresets;
}

void PresetManager::filterPresets (std::vector<chowdsp::Preset>& presets, const ProcessorStore& procStore)
{
    std::erase_if (presets,
                   [&procStore] (const chowdsp::Preset& preset)
                   {
                       const auto* presetXML = preset.getState();
                       jassert (presetXML != nullptr);
                       return ! ProcessorChainStateHelper::validateProcChainState (presetXML, procStore);
                   });
}

std::unique_ptr<XmlElement> PresetManager::savePresetState()
{
    auto xml = procChain->getStateHelper().saveProcChain (true);
    StateManager::setCurrentPluginVersionInXML (xml.get());
    return xml;
}

void PresetManager::loadPresetState (const XmlElement* xml)
{
    if (auto* curPreset = getCurrentPreset())
        Logger::writeToLog ("Loading preset: " + curPreset->getName());

    if (auto* um = vts.undoManager)
    {
        um->beginNewTransaction();
        um->perform (new ChangePresetAction (*this));
    }

    const auto statePluginVersion = StateManager::getPluginVersionFromXML (xml);
    procChain->getStateHelper().loadProcChain (xml, statePluginVersion, true, processor.getActiveEditor());
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
#if BYOD_BUILD_PRESET_SERVER
        PresetInfoHelpers::setIsPublic (*keepAlivePreset, isPublic);
        if (presetID.isNotEmpty())
            PresetInfoHelpers::setPresetID (*keepAlivePreset, presetID);
#else
        ignoreUnused (isPublic, presetID);
#endif

        keepAlivePreset->toFile (getPresetFile (*keepAlivePreset));
        loadPreset (*keepAlivePreset);

        loadUserPresetsFromFolder (getUserPresetPath());
    }
}

void PresetManager::loadUserPresetsFromFolder (const juce::File& file)
{
    std::vector<chowdsp::Preset> presets;
    for (const auto& f : file.findChildFiles (juce::File::findFiles, true, "*" + PresetConstants::presetExt))
    {
        auto newPreset = loadUserPresetFromFile (f);
        if (newPreset.isValid())
            presets.push_back (std::move (newPreset));
    }

    // delete old user presets
    const auto& procStore = procChain->getProcStore();
    sst::cpputils::nodal_erase_if (presetMap, [factoryPresets = getFactoryPresets (procStore)] (const auto& presetPair)
                                   { return ! sst::cpputils::contains (factoryPresets, presetPair.second); });

    int presetID = userIDMap[userPresetsName];
    while (presetMap.find (presetID) != presetMap.end())
        presetMap.erase (presetID++);

    filterPresets (presets, procStore);
    sortPresets (presets);
    addPresets (presets);
}

void PresetManager::loadPresetSafe (std::unique_ptr<chowdsp::Preset> presetToLoad, Component* associatedComp)
{
    if (presetToLoad == nullptr || ! presetToLoad->isValid())
    {
        showErrorMessage ("Preset Load Failure", "Unable to load preset!", associatedComp);
        return;
    }

    keepAlivePreset = std::move (presetToLoad);
    loadPreset (*keepAlivePreset);
}
