#pragma once

#include "state/ParamForwardManager.h"

class BYOD;
class HostContextProvider : private ComponentListener
{
public:
    HostContextProvider (const BYOD& plugin, AudioProcessorEditor* editor);

    std::unique_ptr<HostProvidedContextMenu> getContextMenuForParameter (const RangedAudioParameter*) const;
    void showParameterContextPopupMenu (const RangedAudioParameter*) const;

    void registerParameterComponent (Component& comp, const RangedAudioParameter& param);
    int getParameterIndexForComponent (Component& comp) const;

    const bool supportsParameterModulation;

private:
    void componentBeingDeleted (Component& comp) override;

    const BYOD& plugin;
    AudioProcessorEditor* editor = nullptr;

    const ParamForwardManager& paramForwarder;

    chowdsp::SharedLNFAllocator lnfAllocator;

    std::unordered_map<Component*, int> componentToParameterIndexMap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostContextProvider)
};
