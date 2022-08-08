#pragma once

#include "ProcessorChain.h"

class ProcessorChainActionHelper
{
public:
    explicit ProcessorChainActionHelper (ProcessorChain& thisChain);
    ~ProcessorChainActionHelper();

    void addProcessor (BaseProcessor::Ptr newProc);
    void removeProcessor (BaseProcessor* procToRemove);
    void replaceProcessor (BaseProcessor::Ptr newProc, BaseProcessor* procToReplace);
    void AddProcessorFromCableClick (BaseProcessor::Ptr newProc);
    void clickOnCable (BaseProcessor* startProc, BaseProcessor* endProc);

    void addConnection (ConnectionInfo&& info);
    void removeConnection (ConnectionInfo&& info);

private:
<<<<<<< HEAD
=======
    void hiResTimerCallback() override;

    bool generatedFromCableClick = false;
    BaseProcessor* cableClickStartProc = nullptr;
    BaseProcessor* cableClickEndProc = nullptr;

>>>>>>> Generate new proc off cable click
    ProcessorChain& chain;
    UndoManager* um;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainActionHelper)
};
