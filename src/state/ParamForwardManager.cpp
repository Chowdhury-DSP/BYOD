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

String ParamForwardManager::getForwardingParameterID (int paramNum)
{
    return "forward_param_" + String (paramNum);
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

        if (count == numParams)
        {
            for (int j = numParams; j > 0; --j)
            {
                auto* forwardParam = forwardedParams[i + 1 - j];
                auto* procParam = procParams[numParams - j];

                if (auto* paramCast = dynamic_cast<RangedAudioParameter*> (procParam))
                    forwardParam->setParam (paramCast, proc->getName() + ": " + paramCast->name);
            }

            break;
        }
    }
}

void ParamForwardManager::processorRemoved (const BaseProcessor* proc)
{
    auto& procParams = proc->getParameters();
    for (auto* param : forwardedParams)
    {
        if (auto* internalParam = param->getParam())
        {
            for (auto* procParam : procParams)
            {
                if (procParam == internalParam)
                    param->setParam (nullptr);
            }
        }
    }
}
