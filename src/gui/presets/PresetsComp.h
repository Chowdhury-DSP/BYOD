#pragma once

#include "state/presets/PresetManager.h"

class PresetsComp : public chowdsp::PresetsComp
{
public:
    explicit PresetsComp (chowdsp::PresetManager& presetMgr) : chowdsp::PresetsComp (presetMgr)
    {
        presetListUpdated();
    }

    void presetListUpdated() final
    {
        presetBox.getRootMenu()->clear();

        int optionID = 0;
        optionID = createPresetsMenu (optionID);
        optionID = addPresetOptions (optionID);
        addPresetServerMenuOptions (optionID);
    }

    int addPresetServerMenuOptions (int optionID)
    {
        auto menu = presetBox.getRootMenu();
        menu->addSeparator();

        PopupMenu::Item loginItem { "Login" };
        loginItem.itemID = ++optionID;
        loginItem.action = [=] {

        };
        menu->addItem (loginItem);

        return optionID;
    }

    //    void paint (Graphics& g) override { g.fillAll (Colours::red); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetsComp)
};
