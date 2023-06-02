#pragma once

#include "ProcessorEditor.h"

class BoardComponent;
class EditorSelector : public LassoSource<ProcessorEditor*>
{
public:
    explicit EditorSelector (const BoardComponent&);

    void findLassoItemsInArea (Array<ProcessorEditor*>& results, const Rectangle<int>& area) override;
    SelectedItemSet<ProcessorEditor*>& getLassoSelection() override { return selectedEditorSet; }

private:
    const BoardComponent& board;
    SelectedItemSet<ProcessorEditor*> selectedEditorSet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorSelector)
};
