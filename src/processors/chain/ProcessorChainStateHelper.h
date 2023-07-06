#pragma once

#include "ProcessorChain.h"

class ProcessorChainStateHelper
{
public:
    ProcessorChainStateHelper (ProcessorChain& thisChain, chowdsp::DeferredAction& deferredAction);

    std::unique_ptr<XmlElement> saveProcChain();
    void loadProcChain (const XmlElement* xml,
                        const chowdsp::Version& stateVersion,
                        bool loadingPreset = false,
                        Component* associatedComponent = nullptr,
                        WaitableEvent* waiter = nullptr);

    static bool validateProcChainState (const XmlElement* xml, const ProcessorStore& processorStore);

private:
    void loadProcChainInternal (const XmlElement* xml,
                                const chowdsp::Version& stateVersion,
                                bool loadingPreset,
                                Component* associatedComp);

    ProcessorChain& chain;
    UndoManager* um;

    chowdsp::DeferredAction& mainThreadStateLoader;

    static inline const String procChainStateTag = "proc_chain";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainStateHelper)
};
