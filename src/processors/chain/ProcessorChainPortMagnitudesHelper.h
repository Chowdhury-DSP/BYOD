#pragma once

#include "ProcessorChain.h"

class ProcessorChainPortMagnitudesHelper : private chowdsp::GlobalPluginSettings::Listener,
                                           private ProcessorChain::Listener
{
public:
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;

    explicit ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain);
    ~ProcessorChainPortMagnitudesHelper() override;

    void processorAdded (BaseProcessor* proc) override;

    void globalSettingChanged (SettingID settingID) override;
    void preparePortMagnitudes();

    static constexpr SettingID cableVizOnOffID = "cable_viz_onoff";

private:
    ProcessorChain& chain;

    std::atomic_bool portMagsOn { true };
    bool prevPortMagsOn = true;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainPortMagnitudesHelper)
};
