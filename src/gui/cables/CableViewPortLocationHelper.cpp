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
        if (cable->endProc == sourceProc)
            result |= wouldConnectingCreateFeedbackLoop (cable->startProc, destProc, cables);
    }

    return result;
}

void getClosestPort (const Point<int>& pos, ProcessorEditor* editor, int& minDistance, CableView::EditorPortPair& closestPort, bool wantsInputPort)
{
    int numPorts = wantsInputPort ? editor->getProcPtr()->getNumInputs() : editor->getProcPtr()->getNumOutputs();
    for (int i = 0; i < numPorts; ++i)
    {
        auto portLocation = CableViewPortLocationHelper::getPortLocation (editor, i, wantsInputPort);
        auto distanceFromPort = pos.getDistanceFrom (portLocation);

        bool isClosest = (distanceFromPort < CableConstants::portDistanceLimit && minDistance < 0) || distanceFromPort < minDistance;
        if (isClosest)
        {
            minDistance = distanceFromPort;
            closestPort = std::make_pair (editor, i);
        }
    }
}
} // namespace

CableViewPortLocationHelper::CableViewPortLocationHelper (CableView& cv) : cableView (cv),
                                                                           board (cableView.board),
                                                                           cables (cableView.cables)
{
}

Point<int> CableViewPortLocationHelper::getPortLocation (ProcessorEditor* editor, int portIdx, bool isInput)
{
    auto portLocation = editor->getPortLocation (portIdx, isInput);
    return portLocation + editor->getBounds().getTopLeft();
}

CableView::EditorPortPair CableViewPortLocationHelper::getNearestInputPort (const Point<int>& pos, const BaseProcessor* sourceProc) const
{
    auto result = std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    int minDistance = -1;

    for (auto* editor : board->processorEditors)
    {
        if (wouldConnectingCreateFeedbackLoop (sourceProc, editor->getProcPtr(), cables))
            continue; // no feedback loops allowed

        getClosestPort (pos, editor, minDistance, result, true);
    }

    getClosestPort (pos, board->outputEditor.get(), minDistance, result, true);

    if (result.first == nullptr)
        return result;

    for (auto* cable : cables)
    {
        // the closest port is already connected!
        if (cable->endProc == result.first->getProcPtr() && cable->endIdx == result.second)
            return std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    }

    return result;
}

CableView::EditorPortPair CableViewPortLocationHelper::getNearestOutputPort (const Point<int>& pos) const
{
    auto result = std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    int minDistance = -1;

    for (auto* editor : board->processorEditors)
        getClosestPort (pos, editor, minDistance, result, false);

    getClosestPort (pos, board->inputEditor.get(), minDistance, result, false);

    if (result.first == nullptr)
        return result;

    return result;
}
