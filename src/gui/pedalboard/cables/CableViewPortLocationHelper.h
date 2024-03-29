#pragma once

#include "CableView.h"

class CableViewPortLocationHelper
{
public:
    explicit CableViewPortLocationHelper (CableView& cableView);

    static juce::Point<int> getPortLocation (const CableView::EditorPort& editorPort);
    bool isInputPortConnected (const CableView::EditorPort& editorPort) const;

    CableView::EditorPort getNearestInputPort (const juce::Point<int>& pos, const BaseProcessor* sourceProc) const;
    CableView::EditorPort getNearestPort (const juce::Point<int>& pos, const Component* compUnderMouse = nullptr) const;

private:
    CableView& cableView;
    const BoardComponent& board;
    OwnedArray<Cable>& cables;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableViewPortLocationHelper)
};
