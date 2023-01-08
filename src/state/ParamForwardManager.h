#pragma once

#include "processors/chain/ProcessorChain.h"

class ParamForwardManager : public chowdsp::ForwardingParametersManager<ParamForwardManager, 500>,
                            public chowdsp::TrackedByBroadcasters
{
    using SettingID = chowdsp::GlobalPluginSettings::SettingID;
public:
    ParamForwardManager (AudioProcessorValueTreeState& vts, ProcessorChain& chain);
    ~ParamForwardManager();

    static juce::ParameterID getForwardingParameterID (int paramNum);

    void processorAdded (BaseProcessor* proc);
    void processorRemoved (const BaseProcessor* proc);

    const RangedAudioParameter* getForwardedParameterFromInternal (const RangedAudioParameter& internalParameter) const;

    static constexpr SettingID refreshParamTreeID = "refresh_param_tree"; // IOS+AUv3 only!

private:
    void deferHostNotificationsGlobalSettingChanged (SettingID settingID);

    ProcessorChain& chain;

    chowdsp::ScopedCallbackList callbacks;

    chowdsp::SharedPluginSettings pluginSettings;
    std::optional<ScopedForceDeferHostNotifications> deferHostNotifs {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamForwardManager)
};
