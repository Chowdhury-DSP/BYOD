#pragma once

#include "state/ParamForwardManager.h"

class BYOD;
class HostContextProvider : public chowdsp::HostContextProvider
{
public:
    HostContextProvider (const BYOD& plugin, AudioProcessorEditor& editor);

    std::unique_ptr<HostProvidedContextMenu> getContextMenuForParameter (const RangedAudioParameter&) const override;

    void registerParameterComponent (Component& comp, const RangedAudioParameter& param) override;

private:
    template <typename Action, typename ReturnType = std::invoke_result_t<Action, const RangedAudioParameter&>>
    ReturnType doForParameterOrForwardedParameter (const RangedAudioParameter& param, Action&& action) const;

    const BYOD& plugin;
    const ParamForwardManager& paramForwarder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostContextProvider)
};
