#include "ParamForwardManager.h"

namespace
{
constexpr int maxNumParams = 500;

String getForwardParamID (int paramNum)
{
    return "forward_param_" + String (paramNum);
}
} // namespace

ParamForwardManager::ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& procChain) : chain (procChain)
{
    chain.addListener (this);

    for (int i = 0; i < maxNumParams; ++i)
    {
        auto id = getForwardParamID (i);
        auto forwardedParam = std::make_unique<chowdsp::ForwardingParameter> (id, nullptr, "Blank");

        forwardedParam->setProcessor (&vts.processor);
        forwardedParams.add (forwardedParam.get());
        vts.processor.addParameter (forwardedParam.release());
    }
}

ParamForwardManager::~ParamForwardManager()
{
    chain.removeListener (this);

    for (auto* param : forwardedParams)
        param->setParam (nullptr);
}

void ParamForwardManager::processorAdded (BaseProcessor* proc)
{
    auto& procParams = proc->getParameters();
    const auto numParams = procParams.size();

    // Find a range in forwardedParams with numParams empty params in a row
    int count = 0;
    for (int i = 0; i < forwardedParams.size(); ++i)
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
                {
                    forwardParam->setParam (paramCast, proc->getName() + ": " + paramCast->name);
                    forwardParam->setValueNotifyingHost (paramCast->getValue());
                }
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
