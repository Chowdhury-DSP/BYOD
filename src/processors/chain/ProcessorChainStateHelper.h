#pragma once

#include "ProcessorChain.h"

class ProcessorChainStateHelper : private AsyncUpdater
{
public:
    explicit ProcessorChainStateHelper (ProcessorChain& thisChain);

    void handleAsyncUpdate() override;

    std::unique_ptr<XmlElement> saveProcChain();
    void loadProcChain (const XmlElement* xml, bool loadingPreset = false);

private:
    void loadProcChainInternal (const XmlElement* xml, bool loadingPreset);

    ProcessorChain& chain;
    UndoManager* um;

    CriticalSection crit;
    std::unique_ptr<XmlElement> xmlStateToLoad;
    bool isLoadingPreset = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainStateHelper)
};
