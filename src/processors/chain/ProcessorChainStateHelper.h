#pragma once

#include "ProcessorChain.h"

class ProcessorChainStateHelper
{
public:
    explicit ProcessorChainStateHelper (ProcessorChain& thisChain);

    std::unique_ptr<XmlElement> saveProcChain();
    void loadProcChain (const XmlElement* xml, bool loadingPreset = false);

private:
    void loadProcChainInternal (const XmlElement* xml, bool loadingPreset);

    ProcessorChain& chain;
    UndoManager* um;

    chowdsp::SharedDeferredAction mainThreadStateLoader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainStateHelper)
};
