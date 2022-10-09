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
            popupMenu.showMenuAsync (juce::PopupMenu::Options());
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
