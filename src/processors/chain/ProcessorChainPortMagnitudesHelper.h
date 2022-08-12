#pragma once

#include "ProcessorChain.h"

class ProcessorChainPortMagnitudesHelper : private chowdsp::GlobalPluginSettings::Listener
{
public:
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

    explicit ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain);
    ~ProcessorChainPortMagnitudesHelper() override;

    void processorAdded (BaseProcessor* proc);

    void globalSettingChanged (SettingID settingID) override;
    void preparePortMagnitudes();

    static constexpr SettingID cableVizOnOffID = "cable_viz_onoff";

private:
    ProcessorChain& chain;
    rocket::scoped_connection_container connections;

    std::atomic_bool portMagsOn { true };
    bool prevPortMagsOn = true;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainPortMagnitudesHelper)
};
