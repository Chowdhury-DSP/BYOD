#include "ProcessorEditor.h"

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc) : proc (baseProc)
{
}

void ProcessorEditor::paint (Graphics& g)
{
    g.setColour (proc.getColour());
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}
