#pragma once

#include "CableView.h"

class CableViewConnectionHelper
{
public:
    CableViewConnectionHelper (CableView& cableView, BoardComponent& board);

    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);
    void connectToProcessorChain (ProcessorChain& procChain);

    void addCableToView (Cable* cable);
    void createCable (const ConnectionInfo& connection);
    bool releaseCable (const MouseEvent& e);
    void destroyCable (BaseProcessor* proc, int portIndex);

    void clickOnCable (PopupMenu& menu, PopupMenu::Options& options, Cable* clickedCable);
    std::unique_ptr<MouseEvent> cableMouse;

private:
    void refreshConnections();
    void connectionAdded (const ConnectionInfo& info);
    void connectionRemoved (const ConnectionInfo& info);

    CableView& cableView;
    BoardComponent& board;
    OwnedArray<Cable>& cables;

    bool ignoreConnectionCallbacks = false;

    chowdsp::ScopedCallbackList callbacks;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewConnectionHelper)
};
