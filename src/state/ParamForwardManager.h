#pragma once

#include "processors/chain/ProcessorChain.h"

class ParamForwardManager : private ProcessorChain::Listener
{
public:
    ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& chain);
    ~ParamForwardManager() override;

    void processorAdded (BaseProcessor* proc) override;
    void processorRemoved (const BaseProcessor* proc) override;

private:
    ProcessorChain& chain;

    Array<chowdsp::ForwardingParameter*> forwardedParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamForwardManager)
};
