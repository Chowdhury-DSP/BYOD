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

    const RangedAudioParameter* getForwardedParameterFromInternal (const RangedAudioParameter& internalParameter) const;

private:
    ProcessorChain& chain;

    chowdsp::ScopedCallbackList callbacks;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamForwardManager)
};
