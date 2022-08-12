#pragma once

#include "processors/chain/ProcessorChain.h"

class ParamForwardManager : public chowdsp::ForwardingParametersManager<ParamForwardManager, 500>
{
public:
    ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& chain);
    ~ParamForwardManager();

    static juce::ParameterID getForwardingParameterID (int paramNum);

    void processorAdded (BaseProcessor* proc);
    void processorRemoved (const BaseProcessor* proc);

private:
    ProcessorChain& chain;

    rocket::scoped_connection_container connections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamForwardManager)
};
