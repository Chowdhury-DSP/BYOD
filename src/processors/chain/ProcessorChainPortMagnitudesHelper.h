#pragma once

#include "ProcessorChain.h"

class ProcessorChainPortMagnitudesHelper : private chowdsp::GlobalPluginSettings::Listener,
                                           private ProcessorChain::Listener
{
public:
    explicit ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain);
    ~ProcessorChainPortMagnitudesHelper() override;

    void processorAdded (BaseProcessor* proc) override;

    void propertyChanged (const Identifier& settingID, const var& property) override;
    void preparePortMagnitudes();

    static const Identifier cableVizOnOffID;

private:
    ProcessorChain& chain;

    std::atomic_bool portMagsOn { true };
    bool prevPortMagsOn = true;

    chowdsp::SharedPluginSettings pluginSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainPortMagnitudesHelper)
};
