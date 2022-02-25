#pragma once

#include "CableView.h"

class CableViewConnectionHelper : public ProcessorEditor::PortListener,
                                  public ProcessorChain::Listener
{
public:
    explicit CableViewConnectionHelper (CableView& cableView);

    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);

    void refreshConnections() override;
    void connectionAdded (const ConnectionInfo& info) override;
    void connectionRemoved (const ConnectionInfo& info) override;

    void createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e) override;
    void refreshCable (const MouseEvent& e) override;
    void releaseCable (const MouseEvent& e) override;
    void destroyCable (ProcessorEditor* origin, int portIndex) override;

    std::unique_ptr<MouseEvent> cableMouse;

private:
    CableView& cableView;
    const BoardComponent* board = nullptr;
    OwnedArray<Cable>& cables;

    bool ignoreConnectionCallbacks = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewConnectionHelper)
};
