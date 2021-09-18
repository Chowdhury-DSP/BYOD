#pragma once

#include "Preset.h"

class ProcessorChain;
class PresetComp;
class PresetManager
{
public:
    PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vts);

    StringArray getPresetChoices();
    void loadPresets();
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);

    int getNumPresets() const noexcept { return presets.size(); }
    int getNumFactoryPresets() const noexcept { return numFactoryPresets; }
    String getPresetName (int idx);
    bool setPreset (int idx);
    int getSelectedPresetIdx() const noexcept { return presetParam->get(); }

    void presetUpdated() { listeners.call (&Listener::presetUpdated); }

    File getUserPresetFolder() { return userPresetFolder; }
    void chooseUserPresetFolder();
    bool saveUserPreset (const String& name, int& newPresetIdx);
    const PopupMenu& getUserPresetMenu (const PresetComp* comp) const;

    struct Listener
    {
        virtual ~Listener() {}
        virtual void presetUpdated() {}
    };

    void addListener (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    std::function<void()> hostUpdateFunc = {};

private:
    File getUserPresetConfigFile() const;
    void updateUserPresets();
    void loadPresetFolder (PopupMenu& menu, File& directory);

    File userPresetFolder;
    int numFactoryPresets = 0;
    PopupMenu userPresetMenu;

    ProcessorChain* procChain;
    AudioParameterInt* presetParam = nullptr;

    std::unordered_map<int, Preset*> presetMap;
    OwnedArray<Preset> presets;
    int maxIdx;

    std::future<void> loadingFuture;
    ListenerList<Listener> listeners;

    std::shared_ptr<FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
