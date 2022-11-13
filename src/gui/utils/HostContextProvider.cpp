#include "HostContextProvider.h"
#include "BYOD.h"

HostContextProvider::HostContextProvider (const BYOD& p, AudioProcessorEditor* ed)
    : supportsParameterModulation (p.supportsParameterModulation()),
      plugin (p),
      editor (ed),
      paramForwarder (plugin.getParamForwarder())
{
}

void HostContextProvider::showParameterContextPopupMenu (const RangedAudioParameter* param) const
{
    if (param == nullptr)
        return;

    if (const auto contextMenu = getContextMenuForParameter (param))
    {
        auto popupMenu = contextMenu->getEquivalentPopupMenu();
        if (popupMenu.containsAnyActiveItems())
        {
            const auto options = PopupMenu::Options()
                                     .withParentComponent (editor)
                                     .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards)
                                     .withStandardItemHeight (27);
            popupMenu.setLookAndFeel (lnfAllocator->getLookAndFeel<ByodLNF>());

            popupMenu.showMenuAsync (juce::PopupMenu::Options());
        }
    }
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

std::unique_ptr<HostProvidedContextMenu> HostContextProvider::getContextMenuForParameter (const RangedAudioParameter* param) const
{
    if (param == nullptr)
        return {};

    if (const auto* editorContext = editor->getHostContext())
        return doForParameterOrForwardedParameter (*param, [&editorContext] (auto& p)
                                                   { return editorContext->getContextMenuForParameter (&p); });

    return {};
}

void HostContextProvider::registerParameterComponent (Component& comp, const RangedAudioParameter& param)
{
    doForParameterOrForwardedParameter (param, [this, &comp] (auto& p)
                                        { componentToParameterIndexMap.insert_or_assign (&comp, p.getParameterIndex()); });
}

int HostContextProvider::getParameterIndexForComponent (Component& comp) const
{
    if (const auto iter = componentToParameterIndexMap.find (&comp); iter != componentToParameterIndexMap.end())
        return iter->second;
    return -1;
}

void HostContextProvider::componentBeingDeleted (Component& comp)
{
    componentToParameterIndexMap.erase (&comp);
}
