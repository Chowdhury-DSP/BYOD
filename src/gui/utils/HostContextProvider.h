#pragma once

#include "state/ParamForwardManager.h"

class BYOD;
class HostContextProvider
{
public:
    HostContextProvider (const BYOD& plugin, AudioProcessorEditor* editor);

    std::unique_ptr<HostProvidedContextMenu> getContextMenuForParameter (const RangedAudioParameter*) const;
    void showParameterContextPopupMenu (const RangedAudioParameter*) const;

    const bool supportsParameterModulation;

private:
    const BYOD& plugin;
    AudioProcessorEditor* editor = nullptr;

    const ParamForwardManager& paramForwarder;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostContextProvider)
};
