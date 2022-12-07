#include "HostContextProvider.h"
#include "BYOD.h"
#include "LookAndFeels.h"

HostContextProvider::HostContextProvider (const BYOD& p, AudioProcessorEditor& ed)
    : chowdsp::HostContextProvider (p, ed),
      plugin (p),
      paramForwarder (plugin.getParamForwarder())
{
}

template <typename Action, typename ReturnType>
ReturnType HostContextProvider::doForParameterOrForwardedParameter (const RangedAudioParameter& param, Action&& action) const
{
    if (plugin.getVTS().getParameter (param.paramID) != nullptr)
        return action (param);

    if (const auto* forwardedParam = paramForwarder.getForwardedParameterFromInternal (param))
        return action (*forwardedParam);

    return ReturnType();
}

std::unique_ptr<HostProvidedContextMenu> HostContextProvider::getContextMenuForParameter (const RangedAudioParameter& param) const
{
    return doForParameterOrForwardedParameter (param,
                                               [this] (auto& p) -> std::unique_ptr<HostProvidedContextMenu> {
                                                   return chowdsp::HostContextProvider::getContextMenuForParameter (p);
                                               });
}

void HostContextProvider::registerParameterComponent (Component& comp, const RangedAudioParameter& param)
{
    doForParameterOrForwardedParameter (param,
                                        [this, &comp] (auto& p)
                                        {
                                            chowdsp::HostContextProvider::registerParameterComponent (comp, p);
                                        });
}
