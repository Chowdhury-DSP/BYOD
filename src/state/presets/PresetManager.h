#pragma once

#include "PresetsServerSyncManager.h"
#include "PresetsServerUserManager.h"

class ProcessorChain;
class PresetManager : public chowdsp::PresetManager,
                      private PresetsServerUserManager::Listener
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

    void setUserPresetName (const String& newName) final;
    void saveUserPreset (const String& name, const String& category, bool isPublic);

    void presetLoginStatusChanged() override;

    void syncLocalPresetsToServer() const;
    void syncServerPresetsToLocal (PresetUpdateList& presetsToUpdate);

private:
    ProcessorChain* procChain;

    SharedResourcePointer<PresetsServerUserManager> userManager;
    SharedResourcePointer<PresetsServerSyncManager> syncManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
