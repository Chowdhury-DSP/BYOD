#pragma once

#include "PresetsServerJobPool.h"
#include "PresetsServerSyncManager.h"
#include "PresetsServerUserManager.h"

namespace PresetConstants
{
const String presetExt = ".chowpreset";
const String factoryPresetVendor = "CHOW";
} // namespace PresetConstants

class ProcessorChain;
class PresetManager : public chowdsp::PresetManager
#if BYOD_BUILD_PRESET_SERVER
    ,
                      private PresetsServerUserManager::Listener
#endif
{
public:
    enum PresetUpdate
    {
        Overwriting,
        Adding,
    };

    using PresetUpdateList = std::vector<std::pair<chowdsp::Preset, PresetManager::PresetUpdate>>;

    PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vts);
    ~PresetManager() override;

    std::unique_ptr<XmlElement> savePresetState() override;
    void loadPresetState (const XmlElement* xml) override;
    File getPresetFile (const chowdsp::Preset& preset) const;
    File getPresetFile (const String& vendor, const String& category, const String& name) const;

    void setUserPresetName (const String& newName) final;
    void saveUserPreset (const String& name, const String& category, bool isPublic, const String& presetID = {});
    void loadUserPresetsFromFolder (const juce::File& file) final;

    void loadPresetSafe (std::unique_ptr<chowdsp::Preset> presetToLoad);

#if BYOD_BUILD_PRESET_SERVER
    void presetLoginStatusChanged() override;

    void syncLocalPresetsToServer();
    bool syncServerPresetsToLocal();
    PresetUpdateList& getServerPresetUpdateList() { return serverSyncUpdatePresetsList; };
#endif

private:
    void loadBYODFactoryPresets();
    void filterPresets (std::vector<chowdsp::Preset>& presets);

    ProcessorChain* procChain;

#if BYOD_BUILD_PRESET_SERVER
    SharedPresetsServerUserManager userManager;
    SharedResourcePointer<PresetsServerSyncManager> syncManager;

    SharedPresetsServerJobPool jobPool;
    double jobProgress = 0.0;
    std::unique_ptr<AlertWindow> alertWindow;

    PresetUpdateList serverSyncUpdatePresetsList;
#endif

    friend class ChangePresetAction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
