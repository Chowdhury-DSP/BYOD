#pragma once

#include "ProcessorChain.h"

class AddOrRemoveProcessor : public UndoableAction
{
public:
    AddOrRemoveProcessor (ProcessorChain& procChain, BaseProcessor::Ptr newProc);
    AddOrRemoveProcessor (ProcessorChain& procChain, BaseProcessor* procToRemove);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return (int) sizeof (*this); }

private:
    ProcessorChain& chain;
    BaseProcessor::Ptr actionProc;
    BaseProcessor* actionProcPtr;
    const bool isRemoving;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AddOrRemoveProcessor)
};
