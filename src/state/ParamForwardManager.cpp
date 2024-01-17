#include "ParamForwardManager.h"

ParamForwardManager::ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& procChain)
    : chowdsp::ForwardingParametersManager<ParamForwardManager, 500> (vts),
      chain (procChain)
{
    // In some AUv3 hosts (cough, cough, GarageBand), sending parameter info change notifications
    // causes the host to crash. Since there's no way for the plugin to determine which AUv3
    // host it's running in, we give the user an option to disable these notifications.
    // @TODO: get rid of this option once GarageBand fixes the crash on their end.
    if (vts.processor.wrapperType == AudioProcessor::WrapperType::wrapperType_AudioUnitv3)
    {
        pluginSettings->addProperties<&ParamForwardManager::deferHostNotificationsGlobalSettingChanged> (
            { { refreshParamTreeID, true } }, *this);
        deferHostNotificationsGlobalSettingChanged (refreshParamTreeID);
    }

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

int ParamForwardManager::getNextUnusedParamSlot() const
{
    for (int i = 0; i < numParamSlots; ++i)
        if (! paramSlotUsed[i])
            return i;

    jassertfalse;
    return -1;
}

void ParamForwardManager::processorAdded (BaseProcessor* proc)
{
    auto& procParams = proc->getParameters();
    const auto numParams = procParams.size();

    const auto setForwardParameterRange = [this, &procParams, &proc, numParams] (int slotIndex)
    {
        paramSlotUsed[slotIndex] = true;
        const auto startOffset = slotIndex * maxParameterCount;
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
    };

    if (usingLegacyMode)
    {
        jassert (proc->getForwardingParameterSlotIndex() < 0);

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

                const auto startSlot = startOffset / maxParameterCount;
                const auto endSlot = ((startOffset + numParams) / maxParameterCount);
                std::fill (paramSlotUsed + startSlot, paramSlotUsed + endSlot + 1, true);

                return;
            }
        }
    }

    if (auto slotIndex = proc->getForwardingParameterSlotIndex(); slotIndex >= 0)
    {
        jassert (! paramSlotUsed[slotIndex]);
        setForwardParameterRange (slotIndex);
    }
    else
    {
        slotIndex = getNextUnusedParamSlot();
        if (slotIndex < 0)
        {
            juce::Logger::writeToLog ("Unable to set up forawrding parameters for " + proc->getName() + " - no free slots available!");
            return;
        }

        proc->setForwardingParameterSlotIndex (slotIndex);
        setForwardParameterRange (slotIndex);
    }
}

void ParamForwardManager::processorRemoved (const BaseProcessor* proc)
{
    auto& procParams = proc->getParameters();
    const auto numParams = procParams.size();

    if (const auto slotIndex = proc->getForwardingParameterSlotIndex(); slotIndex >= 0)
    {
        paramSlotUsed[slotIndex] = false;
        const auto startOffset = slotIndex * maxParameterCount;
        clearParameterRange (startOffset, startOffset + numParams);
    }
    else
    {
        int startIndex = -1;
        for (auto [index, param] : sst::cpputils::enumerate (forwardedParams))
        {
            if (auto* internalParam = param->getParam(); internalParam == procParams[0])
            {
                startIndex = static_cast<int> (index);
                clearParameterRange (startIndex, startIndex + numParams);
                break;
            }
        }

        if (startIndex >= 0)
        {
            const auto startSlot = startIndex / maxParameterCount;
            const auto endSlot = ((startIndex + numParams) / maxParameterCount);
            for (int checkSlotIndex = startSlot; checkSlotIndex <= endSlot; ++checkSlotIndex)
            {
                bool slotUsed = false;
                for (int i = checkSlotIndex * maxParameterCount; i < (checkSlotIndex + 1) * maxParameterCount; ++i)
                {
                    if (forwardedParams[i]->getParam() != nullptr)
                    {
                        slotUsed = true;
                        break;
                    }
                }

                if (! slotUsed)
                    paramSlotUsed[checkSlotIndex] = false;
            }
        }
    }
}

void ParamForwardManager::setUsingLegacyMode (bool useLegacy)
{
    usingLegacyMode = useLegacy;
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
