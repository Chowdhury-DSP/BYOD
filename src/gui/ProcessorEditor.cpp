#include "ProcessorEditor.h"

namespace
{
constexpr float cornerSize = 5.0f;
}

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs) : proc (baseProc),
                                                                                    procChain (procs),
                                                                                    procUI (proc.getUIOptions()),
                                                                                    contrastColour (procUI.backgroundColour.contrasting()),
                                                                                    knobs (proc.getVTS(), contrastColour)
{
    addAndMakeVisible (knobs);

    xButton.setButtonText ("x");
    xButton.setColour (TextButton::buttonColourId, Colours::transparentWhite);
    xButton.setColour (ComboBox::outlineColourId, Colours::transparentWhite);
    xButton.setColour (TextButton::textColourOffId, contrastColour);
    xButton.onClick = [=] { MessageManager::callAsync ([=] {
        procChain.removeProcessor (&proc);
    });};
    addAndMakeVisible (xButton);

    if (procUI.lnf != nullptr)
        setLookAndFeel (procUI.lnf);
}

ProcessorEditor::~ProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ProcessorEditor::paint (Graphics& g)
{
    const auto& procColour = procUI.backgroundColour;
    g.setColour (procColour);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), cornerSize);

    if (procUI.backgroundImage != nullptr)
    {
        auto backgroundBounds = getLocalBounds().reduced ((int) cornerSize);
        procUI.backgroundImage->drawWithin (g, backgroundBounds.toFloat(), RectanglePlacement::stretchToFit, 1.0f);
    }

    g.setColour (contrastColour);
    g.setFont (Font (25.0f).boldened());
    g.drawFittedText (proc.getName(), 5, 0, getWidth() - 50, 30, Justification::centredLeft, 1);
}

void ProcessorEditor::resized()
{
    knobs.setBounds (5, 35, getWidth() - 10, getHeight() - 40);

    const int xButtonSize = 30;
    xButton.setBounds (getWidth() - xButtonSize, 0, xButtonSize, xButtonSize);
}
