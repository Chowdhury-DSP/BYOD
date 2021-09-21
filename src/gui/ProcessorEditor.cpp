#include "ProcessorEditor.h"
#include "BoardComponent.h"

namespace
{
constexpr float cornerSize = 5.0f;
}

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs, Component* parent) : proc (baseProc),
                                                                                                       procChain (procs),
                                                                                                       procUI (proc.getUIOptions()),
                                                                                                       contrastColour (procUI.backgroundColour.contrasting()),
                                                                                                       knobs (baseProc, proc.getVTS(), contrastColour, procUI.powerColour),
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
    xButton.onClick = [=]
    { MessageManager::callAsync ([=]
                                 { procChain.removeProcessor (&proc); }); };
    addAndMakeVisible (xButton);

    auto infoSvg = Drawable::createFromImageData (BinaryData::info_svg, BinaryData::info_svgSize);
    infoSvg->replaceColour (Colours::black, contrastColour);
    infoButton.setImages (infoSvg.get());
    addAndMakeVisible (infoButton);
    infoButton.onClick = [&baseProc, boardComp = dynamic_cast<BoardComponent*> (parent)]
    {
        boardComp->showInfoComp (baseProc);
    };

    if (procUI.lnf != nullptr)
        setLookAndFeel (procUI.lnf);

    for (int i = 0; i < baseProc.getNumInputs(); ++i)
    {
        auto newPort = inputPorts.add (std::make_unique<Port>());
        newPort->setInputOutput (true);
        addAndMakeVisible (newPort);
        newPort->addPortListener (this);
    }

    for (int i = 0; i < baseProc.getNumOutputs(); ++i)
    {
        auto newPort = outputPorts.add (std::make_unique<Port>());
        newPort->setInputOutput (false);
        addAndMakeVisible (newPort);
        newPort->addPortListener (this);
    }
}

ProcessorEditor::~ProcessorEditor()
{
    setLookAndFeel (nullptr);

    for (auto* port : inputPorts)
        port->removePortListener (this);

    for (auto* port : outputPorts)
        port->removePortListener (this);
}

void ProcessorEditor::paint (Graphics& g)
{
    const auto& procColour = procUI.backgroundColour;
    ColourGradient grad { procColour,
                          0.0f,
                          0.0f,
                          procColour.darker (0.25f),
                          (float) getWidth(),
                          (float) getWidth(),
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
    const auto width = getWidth();
    const auto height = getHeight();
    knobs.setBounds (5, 35, width - 10, height - 40);

    bool isIOProcessor = typeid (proc) == typeid (InputProcessor) || typeid (proc) == typeid (OutputProcessor);
    if (! isIOProcessor)
    {
        constexpr int xButtonSize = 27;
        powerButton.setBounds (width - 2 * xButtonSize, 0, xButtonSize, xButtonSize);
        xButton.setBounds (width - xButtonSize, 0, xButtonSize, xButtonSize);

        constexpr int infoButtonSize = 20;
        infoButton.setBounds (width - infoButtonSize, height - infoButtonSize, infoButtonSize, infoButtonSize);
    }

    const int portDim = height / 8;
    auto placePorts = [=] (int x, auto& ports)
    {
        const auto nPorts = ports.size();
        if (nPorts == 0)
            return;

        const auto yPad = height / nPorts;
        int y = yPad / 2;
        for (auto* port : ports)
        {
            port->setBounds (x, y, portDim, portDim);
            y += yPad;
        }
    };

    placePorts (-portDim / 2, inputPorts);
    placePorts (width - portDim / 2, outputPorts);
}

void ProcessorEditor::mouseDrag (const MouseEvent& e)
{
    const auto relE = e.getEventRelativeTo (getParentComponent());

    if (auto* parent = getParentComponent())
    {
        auto parentBounds = parent->getBounds();
        proc.setPosition (relE.getPosition(), parentBounds);
        setTopLeftPosition (proc.getPosition (parentBounds));
        getParentComponent()->repaint();
    }
    else
    {
        jassertfalse;
    }
}

void ProcessorEditor::createCable (Port* origin, const MouseEvent& e)
{
    portListeners.call (&PortListener::createCable, this, outputPorts.indexOf (origin), e);
}

void ProcessorEditor::refreshCable (const MouseEvent& e)
{
    portListeners.call (&PortListener::refreshCable, e);
}

void ProcessorEditor::releaseCable (const MouseEvent& e)
{
    portListeners.call (&PortListener::releaseCable, e);
}

void ProcessorEditor::destroyCable (Port* origin)
{
    portListeners.call (&PortListener::destroyCable, this, inputPorts.indexOf (origin));
}

Point<int> ProcessorEditor::getPortLocation (int portIndex, bool isInput) const
{
    if (isInput)
    {
        jassert (portIndex < inputPorts.size());
        return inputPorts[portIndex]->getBounds().getCentre();
    }

    jassert (portIndex < outputPorts.size());
    return outputPorts[portIndex]->getBounds().getCentre();
}
