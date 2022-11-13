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

std::unique_ptr<HostProvidedContextMenu> HostContextProvider::getContextMenuForParameter (const RangedAudioParameter* param) const
{
    if (param == nullptr)
        return {};

    if (const auto* editorContext = editor->getHostContext())
    {
        if (plugin.getVTS().getParameter (param->paramID) != nullptr)
            return editorContext->getContextMenuForParameter (param);

        if (const auto* forwardedParam = paramForwarder.getForwardedParameterFromInternal (*param))
            return editorContext->getContextMenuForParameter (forwardedParam);
    }
    return {};
}

void HostContextProvider::registerParameterComponent (Component& comp, const RangedAudioParameter& param)
{
    componentToParameterIndexMap.insert_or_assign (&comp, param.getParameterIndex());
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
