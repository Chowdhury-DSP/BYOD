#pragma once

#include "ProcessorChain.h"

class ProcessorChainPortMagnitudesHelper : public chowdsp::TrackedByBroadcasters
{
public:
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

    explicit ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain);

    void processorAdded (BaseProcessor* proc);

    void globalSettingChanged (SettingID settingID);
    void preparePortMagnitudes();

    static constexpr SettingID cableVizOnOffID = "cable_viz_onoff";

private:
    ProcessorChain& chain;
    chowdsp::ScopedCallbackList callbacks;

    std::atomic_bool portMagsOn { true };
    bool prevPortMagsOn = true;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainPortMagnitudesHelper)
};
