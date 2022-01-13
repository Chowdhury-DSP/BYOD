#pragma once

#include "PresetsServerSyncManager.h"
#include "PresetsServerUserManager.h"

class ProcessorChain;
class PresetManager : public chowdsp::PresetManager,
                      private PresetsServerUserManager::Listener
{
public:
    PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vts);
    ~PresetManager() override;

    std::unique_ptr<XmlElement> savePresetState() override;
    void loadPresetState (const XmlElement* xml) override;

    void presetLoginStatusChanged() override;

    void syncLocalPresetsToServer() const;

private:
    ProcessorChain* procChain;

    SharedResourcePointer<PresetsServerUserManager> userManager;
    SharedResourcePointer<PresetsServerSyncManager> syncManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
