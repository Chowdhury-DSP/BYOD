#pragma once

#include "processors/chain/ProcessorChain.h"

class ParamForwardManager : public chowdsp::ForwardingParametersManager<ParamForwardManager, 500>,
                            private ProcessorChain::Listener
{
public:
    ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& chain);
    ~ParamForwardManager() override;

    static String getForwardingParameterID (int paramNum);

    void processorAdded (BaseProcessor* proc) override;
    void processorRemoved (const BaseProcessor* proc) override;

private:
    ProcessorChain& chain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamForwardManager)
};
