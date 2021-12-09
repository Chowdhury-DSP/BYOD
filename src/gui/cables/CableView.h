#pragma once

#include "../editors/ProcessorEditor.h"
#include "Cable.h"

namespace CableConstants
{
const Colour cableColour (0xFFD0592C);
constexpr float cableThickness = 5.0f;
} // namespace CableConstants

class BoardComponent;
class CableView : public Component,
                  public ProcessorEditor::PortListener,
                  public ProcessorChain::Listener
{
public:
    explicit CableView (const BoardComponent* comp);

    void paint (Graphics& g) override;

    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);
    void refreshConnections() override;
    void connectionAdded (const ConnectionInfo& info) override;
    void connectionRemoved (const ConnectionInfo& info) override;

    void createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e) override;
    void refreshCable (const MouseEvent& e) override;
    void releaseCable (const MouseEvent& e) override;
    void destroyCable (ProcessorEditor* origin, int portIndex) override;

private:
    std::pair<ProcessorEditor*, int> getNearestInputPort (const Point<int>& pos) const;

    const BoardComponent* board = nullptr;

    OwnedArray<Cable> cables;
    std::unique_ptr<MouseEvent> cableMouse;
    bool ignoreConnectionCallbacks = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableView)
};
