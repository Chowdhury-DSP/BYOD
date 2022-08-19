#pragma once

#include "CableView.h"

class CableViewConnectionHelper
{
public:
    explicit CableViewConnectionHelper (CableView& cableView);

    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);
    void connectToProcessorChain (ProcessorChain& procChain);

    void createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e);
    void refreshCable (const MouseEvent& e);
    void releaseCable (const MouseEvent& e);
    void destroyCable (ProcessorEditor* origin, int portIndex);

    std::unique_ptr<MouseEvent> cableMouse;

private:
    void refreshConnections();
    void connectionAdded (const ConnectionInfo& info);
    void connectionRemoved (const ConnectionInfo& info);

    CableView& cableView;
    const BoardComponent* board = nullptr;
    OwnedArray<Cable>& cables;

    bool ignoreConnectionCallbacks = false;

    chowdsp::ScopedCallbackList callbacks;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewConnectionHelper)
};
