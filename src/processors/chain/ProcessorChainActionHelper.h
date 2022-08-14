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
    void replaceConnectionWithProcessor (BaseProcessor::Ptr newProc, ConnectionInfo& connectionInfo);

    void addConnection (ConnectionInfo&& info);
    void removeConnection (ConnectionInfo&& info);

private:
<<<<<<< HEAD
=======
    void hiResTimerCallback() override;

<<<<<<< HEAD
    bool generatedFromCableClick = false;
    BaseProcessor* cableClickStartProc = nullptr;
    BaseProcessor* cableClickEndProc = nullptr;

>>>>>>> Generate new proc off cable click
=======
>>>>>>> PR changes, improved cable click menu
    ProcessorChain& chain;
    UndoManager* um;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainActionHelper)
};
