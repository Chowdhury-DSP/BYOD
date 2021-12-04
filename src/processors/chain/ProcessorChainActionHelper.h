#pragma once

#include "ProcessorChain.h"

class ProcessorChainActionHelper
{
public:
    explicit ProcessorChainActionHelper (ProcessorChain& thisChain);

    void addProcessor (BaseProcessor::Ptr newProc);
    void removeProcessor (BaseProcessor* procToRemove);
    void replaceProcessor (BaseProcessor::Ptr newProc, BaseProcessor* procToReplace);

    void addConnection (ConnectionInfo&& info);
    void removeConnection (ConnectionInfo&& info);

private:
    ProcessorChain& chain;
    UndoManager* um;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainActionHelper)
};
