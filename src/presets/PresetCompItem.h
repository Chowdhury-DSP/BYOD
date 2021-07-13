#pragma once

#include "BYOD.h"
#include "PresetComp.h"

class PresetCompItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY (PresetCompItem)

    PresetCompItem (foleys::MagicGUIBuilder& builder, const ValueTree& node) : foleys::GuiItem (builder, node)
    {
        setColourTranslation ({ { "presets-background", PresetComp::backgroundColourId },
                                { "presets-text", PresetComp::textColourId } });

        if (auto* proc = dynamic_cast<BYOD*> (builder.getMagicState().getProcessor()))
        {
            presetComp = std::make_unique<PresetComp> (proc->getPresetManager());
            addAndMakeVisible (presetComp.get());
        }
    }

    void update() override
    {
        presetComp->resized();
    }

    Component* getWrappedComponent() override
    {
        return presetComp.get();
    }

private:
    std::unique_ptr<PresetComp> presetComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetCompItem)
};