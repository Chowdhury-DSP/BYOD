#pragma once

#include "CableView.h"

class CableViewConnectionHelper : public ProcessorChain::Listener
{
public:
    explicit CableViewConnectionHelper (CableView& cableView);

    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingAddedFromCableClick (BaseProcessor* newProc, Cable* c); //rename this?
    void processorBeingRemoved (const BaseProcessor* proc);

    void refreshConnections() override;
    void connectionAdded (const ConnectionInfo& info) override;
    void connectionRemoved (const ConnectionInfo& info) override;

    void addCableToView (Cable* cable);
    void createCable (const ConnectionInfo& connection);
    void releaseCable (const MouseEvent& e);
    void destroyCable (BaseProcessor* proc, int portIndex);

    void clickOnCable (Cable* clickedCable);

    std::unique_ptr<MouseEvent> cableMouse;

private:
    CableView& cableView;
    const BoardComponent* board = nullptr;
    OwnedArray<Cable>& cables;

    bool ignoreConnectionCallbacks = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewConnectionHelper)
};
