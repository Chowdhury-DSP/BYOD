#pragma once

#include "CableView.h"

class CableViewPortLocationHelper
{
public:
    explicit CableViewPortLocationHelper (CableView& cableView);

    static Point<int> getPortLocation (ProcessorEditor* editor, int portIdx, bool isInput);

    CableView::EditorPortPair getNearestInputPort (const Point<int>& pos, const BaseProcessor* sourceProc) const;
    CableView::EditorPortPair getNearestOutputPort (const Point<int>& pos) const;

private:
    CableView& cableView;
    const BoardComponent* board = nullptr;
    OwnedArray<Cable>& cables;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewPortLocationHelper)
};
