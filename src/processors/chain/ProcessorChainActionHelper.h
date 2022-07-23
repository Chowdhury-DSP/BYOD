#pragma once

#include "ProcessorChain.h"

class ProcessorChainActionHelper : private HighResolutionTimer
{
public:
    explicit ProcessorChainActionHelper (ProcessorChain& thisChain);
    ~ProcessorChainActionHelper() override;

    void addProcessor (BaseProcessor::Ptr newProc);
    void removeProcessor (BaseProcessor* procToRemove);
    void replaceProcessor (BaseProcessor::Ptr newProc, BaseProcessor* procToReplace);

    void addConnection (ConnectionInfo&& info);
    void removeConnection (ConnectionInfo&& info);

    void processActions();

private:
    void hiResTimerCallback() override;

    ProcessorChain& chain;
    UndoManager* um;

    using Action = dsp::FixedSizeFunction<64, void()>;
    moodycamel::ReaderWriterQueue<Action> actionQueue { 100 };

    friend class ProcChainActions;

    int timersSinceLastProcess = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorChainActionHelper)
};