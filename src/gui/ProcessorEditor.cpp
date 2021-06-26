#include "ProcessorEditor.h"
#include "BoardComponent.h"

namespace
{
constexpr float cornerSize = 5.0f;
}

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs, Component* parent) :
    proc (baseProc),
    procChain (procs),
    procUI (proc.getUIOptions()),
    contrastColour (procUI.backgroundColour.contrasting()),
    knobs (proc.getVTS(), contrastColour),
    powerButton (procUI.powerColour)
{
    addAndMakeVisible (knobs);

    addAndMakeVisible (powerButton);
    powerButton.setEnableDisableComps ({ &knobs });
    powerButton.attachButton (proc.getVTS(), "on_off");

    xButton.setButtonText ("x");
    xButton.setColour (TextButton::buttonColourId, Colours::transparentWhite);
    xButton.setColour (ComboBox::outlineColourId, Colours::transparentWhite);
    xButton.setColour (TextButton::textColourOffId, contrastColour);
    xButton.onClick = [=] { MessageManager::callAsync ([=] {
        procChain.removeProcessor (&proc);
    });};
    addAndMakeVisible (xButton);

    auto infoSvg = Drawable::createFromImageData (BinaryData::info_svg, BinaryData::info_svgSize);
    infoSvg->replaceColour (Colours::black, contrastColour);
    infoButton.setImages (infoSvg.get());
    addAndMakeVisible (infoButton);
    infoButton.onClick = [&baseProc, boardComp = dynamic_cast<BoardComponent*> (parent)] {
        boardComp->showInfoComp (baseProc);
    };

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
    ColourGradient grad { procColour,
                          0.0f, 0.0f,
                          procColour.darker (0.25f),
                          (float) getWidth(), (float) getWidth(),
                          false };
    g.setGradientFill (grad);
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

    constexpr int xButtonSize = 27;
    powerButton.setBounds (getWidth() - 2 * xButtonSize, 0, xButtonSize, xButtonSize);
    xButton.setBounds (getWidth() - xButtonSize, 0, xButtonSize, xButtonSize);

    constexpr int infoButtonSize = 20;
    infoButton.setBounds (getWidth() - infoButtonSize, getHeight() - infoButtonSize, infoButtonSize, infoButtonSize);
}

void ProcessorEditor::mouseDrag (const MouseEvent&)
{
    if (auto* dragC = DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (! dragC->isDragAndDropActive())
          dragC->startDragging("ProcessorEditor", this);
    }
}
