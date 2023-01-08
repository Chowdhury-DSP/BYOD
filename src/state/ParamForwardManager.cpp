#include "ParamForwardManager.h"

ParamForwardManager::ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& procChain) : chowdsp::ForwardingParametersManager<ParamForwardManager, 500> (vts),
                                                                                                          chain (procChain)
{
    // In some AUv3 hosts (cough, cough, GarageBand), sending parameter info change notifications
    // causes the host to crash. Since there's no way for the plugin to determine which AUv3
    // host it's running in, we give the user an option to disable these notifications.
    // @TODO: get rid of this option once GarageBand fixes the crash on their end.
    if (vts.processor.wrapperType == AudioProcessor::WrapperType::wrapperType_AudioUnitv3)
        pluginSettings->addProperties<&ParamForwardManager::deferHostNotificationsGlobalSettingChanged> (
            { { refreshParamTreeID, true } }, *this);
    deferHostNotificationsGlobalSettingChanged (refreshParamTreeID);

    callbacks += {
        chain.processorAddedBroadcaster.connect<&ParamForwardManager::processorAdded> (this),
        chain.processorRemovedBroadcaster.connect<&ParamForwardManager::processorRemoved> (this),
    };
}

ParamForwardManager::~ParamForwardManager() = default;

juce::ParameterID ParamForwardManager::getForwardingParameterID (int paramNum)
{
    // if we decide to increase the number of forwarding parameters in the future,
    // make sure to use a different version tag for the new parameters!
    return { "forward_param_" + String (paramNum), 100 };
}

const RangedAudioParameter* ParamForwardManager::getForwardedParameterFromInternal (const RangedAudioParameter& internalParameter) const
{
    if (const auto paramIter = std::find_if (forwardedParams.begin(),
                                             forwardedParams.end(),
                                             [&internalParameter] (const auto* fParam)
                                             {
                                                 return fParam->getParam() == &internalParameter;
                                             });
        paramIter != forwardedParams.end())
    {
        return *paramIter;
    }

    return nullptr;
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

void ParamForwardManager::deferHostNotificationsGlobalSettingChanged (SettingID settingID)
{
    if (settingID != refreshParamTreeID)
        return;

    if (pluginSettings->getProperty<bool> (refreshParamTreeID))
        deferHostNotifs.reset();
    else
        deferHostNotifs.emplace (*this);
}
