#include "ParamForwardManager.h"

ParamForwardManager::ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& procChain) : chowdsp::ForwardingParametersManager<ParamForwardManager, 500> (vts),
                                                                                                          chain (procChain)
{
    chain.addListener (this);
}

ParamForwardManager::~ParamForwardManager()
{
    chain.removeListener (this);
}

juce::ParameterID ParamForwardManager::getForwardingParameterID (int paramNum)
{
    // if we decide to increase the number of forwarding parameters in the future,
    // make sure to use a different version tag for the new parameters!
    return { "forward_param_" + String (paramNum), 100 };
}

void ParamForwardManager::processorAdded (BaseProcessor* proc)
{
    auto& procParams = proc->getParameters();
    const auto numParams = procParams.size();

    // Find a range in forwardedParams with numParams empty params in a row
    int count = 0;
    for (int i = 0; i < (int) forwardedParams.size(); ++i)
    {
        if (forwardedParams[i]->getParam() == nullptr)
            count++;
        else
            count = 0;

        if (count == numParams)
        {
            int startOffset = i + 1 - numParams;
            setParameterRange (startOffset,
                               startOffset + numParams,
                               [&procParams, &proc, startOffset] (int index) -> chowdsp::ParameterForwardingInfo
                               {
                                   auto* procParam = procParams[index - startOffset];

                                   if (auto* paramCast = dynamic_cast<RangedAudioParameter*> (procParam))
                                       return { paramCast, proc->getName() + ": " + paramCast->name };

                                   jassertfalse;
                                   return {};
                               });

            break;
        }
    }
}

void ParamForwardManager::processorRemoved (const BaseProcessor* proc)
{
    auto& procParams = proc->getParameters();

    for (auto [index, param] : sst::cpputils::enumerate (forwardedParams))
    {
        if (auto* internalParam = param->getParam(); internalParam == procParams[0])
        {
            clearParameterRange ((int) index, (int) index + procParams.size());
            break;
        }
    }
}
