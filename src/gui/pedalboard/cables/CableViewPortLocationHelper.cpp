#include "CableViewPortLocationHelper.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"

namespace
{
bool wouldConnectingCreateFeedbackLoop (const BaseProcessor* sourceProc, const BaseProcessor* destProc, const OwnedArray<Cable>& cables)
{
    if (sourceProc->getNumInputs() == 0)
        return false;

    if (sourceProc == destProc)
        return true;

    bool result = false;
    for (auto* cable : cables)
    {
        if (cable->connectionInfo.endProc == sourceProc)
            result |= wouldConnectingCreateFeedbackLoop (cable->connectionInfo.startProc, destProc, cables);
    }

    return result;
}

void getClosestPort (const juce::Point<int>& pos, ProcessorEditor* editor, int& minDistance, CableView::EditorPort& closestPort, bool wantsInputPort, float scaleFactor)
{
    int numPorts = wantsInputPort ? editor->getProcPtr()->getNumInputs() : editor->getProcPtr()->getNumOutputs();
    const auto portDistanceLimit = CableConstants::getPortDistanceLimit (scaleFactor);
    for (int i = 0; i < numPorts; ++i)
    {
        auto portLocation = CableViewPortLocationHelper::getPortLocation ({ editor, i, wantsInputPort });
        auto distanceFromPort = pos.getDistanceFrom (portLocation);

        bool isClosest = (distanceFromPort < portDistanceLimit && minDistance < 0) || distanceFromPort < minDistance;
        if (isClosest)
        {
            minDistance = distanceFromPort;
            closestPort = CableView::EditorPort { editor, i, wantsInputPort };
        }
    }
}
} // namespace

CableViewPortLocationHelper::CableViewPortLocationHelper (CableView& cv) : cableView (cv),
                                                                           board (cableView.board),
                                                                           cables (cableView.cables)
{
}

juce::Point<int> CableViewPortLocationHelper::getPortLocation (const CableView::EditorPort& editorPort)
{
    auto portLocation = editorPort.editor->getPortLocation (editorPort.portIndex, editorPort.isInput);
    return portLocation + editorPort.editor->getBounds().getTopLeft();
}

bool CableViewPortLocationHelper::isInputPortConnected (const CableView::EditorPort& editorPort) const
{
    return sst::cpputils::contains_if (cables, [&editorPort] (auto* cable)
                                       { return cable->connectionInfo.endProc == editorPort.editor->getProcPtr() && cable->connectionInfo.endPort == editorPort.portIndex; });
}

CableView::EditorPort CableViewPortLocationHelper::getNearestInputPort (const juce::Point<int>& pos, const BaseProcessor* sourceProc) const
{
    auto result = CableView::EditorPort {};
    int minDistance = -1;

    for (auto* editor : board->processorEditors)
    {
        if (wouldConnectingCreateFeedbackLoop (sourceProc, editor->getProcPtr(), cables))
            continue; // no feedback loops allowed

        getClosestPort (pos, editor, minDistance, result, true, cableView.scaleFactor);
    }

    getClosestPort (pos, board->outputEditor.get(), minDistance, result, true, cableView.scaleFactor);

    if (result.editor == nullptr || isInputPortConnected (result))
        return {};

    return result;
}

CableView::EditorPort CableViewPortLocationHelper::getNearestPort (const juce::Point<int>& pos, const Component* compUnderMouse) const
{
    auto result = CableView::EditorPort {};
    int minDistance = -1;

    for (auto* editor : board->processorEditors)
    {
        getClosestPort (pos, editor, minDistance, result, false, cableView.scaleFactor);
        getClosestPort (pos, editor, minDistance, result, true, cableView.scaleFactor);
    }

    getClosestPort (pos, board->inputEditor.get(), minDistance, result, false, cableView.scaleFactor);
    getClosestPort (pos, board->outputEditor.get(), minDistance, result, true, cableView.scaleFactor);

    if (result.editor == nullptr)
        return {};

    if (compUnderMouse != nullptr)
    {
        const auto* port = result.editor->getPort (result.portIndex, result.isInput);
        if (! sst::cpputils::contains (std::array<const Component*, 3> { board, result.editor, port }, compUnderMouse))
            return {}; // wrong component under the mouse!
    }

    return result;
}
