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

class MoveProcessor : public UndoableAction
{
public:
    MoveProcessor (ProcessorChain& procChain, int indexToMove, int slotIndex);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return (int) sizeof (*this); }
    UndoableAction* createCoalescedAction (UndoableAction* nextAction) override;

private:
    ProcessorChain& chain;
    const int startIndex, endIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoveProcessor)
};
