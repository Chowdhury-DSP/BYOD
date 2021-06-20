#include "ProcessorEditor.h"

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs) : proc (baseProc),
                                                                                    procChain (procs),
                                                                                    knobs (proc.getVTS())
{
    addAndMakeVisible (knobs);

    xButton.setButtonText ("x");
    xButton.setColour (TextButton::buttonColourId, Colours::transparentWhite);
    xButton.setColour (ComboBox::outlineColourId, Colours::transparentWhite);
    xButton.onClick = [=] { MessageManager::callAsync ([=] {
        procChain.removeProcessor (&proc);
    });};
    addAndMakeVisible (xButton);
}

void ProcessorEditor::paint (Graphics& g)
{
    const auto& procColour = proc.getColour();
    g.setColour (procColour);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);

    g.setColour (procColour.contrasting());
    g.setFont (Font (25.0f).boldened());
    g.drawFittedText (proc.getName(), 5, 0, 100, 30, Justification::centredLeft, 1);
}

void ProcessorEditor::resized()
{
    knobs.setBounds (5, 35, getWidth() - 10, getHeight() - 40);

    const int xButtonSize = 30;
    xButton.setBounds (getWidth() - xButtonSize, 0, xButtonSize, xButtonSize);
}
