#include "EditorSelector.h"
#include "../BoardComponent.h"

EditorSelector::EditorSelector (const BoardComponent& boardComp) : board (boardComp)
{
}

void EditorSelector::findLassoItemsInArea (Array<ProcessorEditor*>& results, const Rectangle<int>& area)
{
    const auto checkAndAddEditor = [&results, area] (ProcessorEditor* editor)
    {
        if (area.intersects (editor->getBoundsInParent()))
            results.add (editor);
    };

    for (auto* editor : board.getEditors())
        checkAndAddEditor (editor);
    checkAndAddEditor (board.getInputProcessorEditor());
    checkAndAddEditor (board.getOutputProcessorEditor());
}
