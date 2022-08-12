#pragma once

#include "CableView.h"

class CableViewConnectionHelper
{
public:
    explicit CableViewConnectionHelper (CableView& cableView);

    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);

    void refreshConnections();
    void connectionAdded (const ConnectionInfo& info);
    void connectionRemoved (const ConnectionInfo& info);
    auto& getCallbackConnections() { return connections; }

    void createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e);
    void refreshCable (const MouseEvent& e);
    void releaseCable (const MouseEvent& e);
    void destroyCable (ProcessorEditor* origin, int portIndex);

    std::unique_ptr<MouseEvent> cableMouse;

private:
    CableView& cableView;
    const BoardComponent* board = nullptr;
    OwnedArray<Cable>& cables;

    bool ignoreConnectionCallbacks = false;

    rocket::scoped_connection_container connections; // @TODO: having this named "connections" is super confusing!

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewConnectionHelper)
};
