#include "ProcessorEditor.h"
#include "../BoardComponent.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
constexpr float cornerSize = 5.0f;
}

ProcessorEditor::ProcessorEditor (BaseProcessor& baseProc, ProcessorChain& procs) : proc (baseProc),
                                                                                    procChain (procs),
                                                                                    procUI (proc.getUIOptions()),
                                                                                    contrastColour (procUI.backgroundColour.contrasting()),
                                                                                    knobs (baseProc, proc.getVTS(), contrastColour, procUI.powerColour),
                                                                                    powerButton (procUI.powerColour)
{
    addAndMakeVisible (knobs);
    setBroughtToFrontOnMouseClick (true);

    addAndMakeVisible (powerButton);
    powerButton.setEnableDisableComps ({ &knobs });
    powerButton.attachButton (proc.getVTS(), "on_off");

    auto xSvg = Drawable::createFromImageData (BinaryData::xsolid_svg, BinaryData::xsolid_svgSize);
    xSvg->replaceColour (Colours::white, contrastColour);
    xButton.setImages (xSvg.get());
    addAndMakeVisible (xButton);
    xButton.onClick = [=]
    { MessageManager::callAsync ([=]
                                 { procChain.getActionHelper().removeProcessor (&proc); }); };

    if (procUI.lnf != nullptr)
        setLookAndFeel (procUI.lnf);
    else
        setLookAndFeel (lnfAllocator->getLookAndFeel<ProcessorLNF>());

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

    popupMenu.setAssociatedComponent (this);
    popupMenu.popupMenuCallback = [&] (PopupMenu& menu, PopupMenu::Options& options)
    {
        if (proc.getNumInputs() == 0 || proc.getNumOutputs() == 0)
            return; // no menu for I/O processors

        menu.addItem ("Reset", [&]
                      { resetProcParameters(); });

        PopupMenu replaceProcMenu;
        createReplaceProcMenu (replaceProcMenu);
        menu.addSubMenu ("Replace", replaceProcMenu);

        if (auto* p = getParentComponent())
        {
            menu.addItem ("Info", [&, boardComp = dynamic_cast<BoardComponent*> (p)]
                          { boardComp->showInfoComp (proc); });

            options = options.withParentComponent (p);
        }

        menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ProcessorLNF>());

        options = options.withStandardItemHeight (27);
    };
}

ProcessorEditor::~ProcessorEditor()
{
    for (auto* port : inputPorts)
        port->removePortListener (this);

    for (auto* port : outputPorts)
        port->removePortListener (this);
}

void ProcessorEditor::resetProcParameters()
{
    auto& vts = proc.getVTS();
    if (auto* um = vts.undoManager)
        um->beginNewTransaction();

    for (auto* param : proc.getVTS().processor.getParameters())
    {
        if (auto* rangedParam = dynamic_cast<RangedAudioParameter*> (param))
        {
            if (rangedParam->paramID == "on_off")
                continue;

            rangedParam->setValueNotifyingHost (rangedParam->getDefaultValue());
        }
    }
}

void ProcessorEditor::createReplaceProcMenu (PopupMenu& menu)
{
    auto& procStore = procChain.getProcStore();

    int menuID = 100;
    for (auto type : { Drive, Tone, Utility, Other })
    {
        PopupMenu subMenu;
        procStore.createProcReplaceList (subMenu, menuID, type, &proc);

        auto typeName = std::string (magic_enum::enum_name (type));
        menu.addSubMenu (String (typeName), subMenu);
    }
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

    auto fontHeight = proportionOfHeight (0.139f);
    auto nameHeight = proportionOfHeight (0.167f);

    g.setColour (contrastColour);
    g.setFont (Font ((float) fontHeight).boldened());
    g.drawFittedText (proc.getName(), 5, 0, jmax (getWidth() - 50, 100), nameHeight, Justification::centredLeft, 1);
}

void ProcessorEditor::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto knobsPad = proportionOfWidth (0.015f);
    auto nameHeight = proportionOfHeight (0.167f);
    knobs.setBounds (knobsPad, nameHeight, width - 2 * knobsPad, height - (nameHeight + knobsPad));

    bool isIOProcessor = typeid (proc) == typeid (InputProcessor) || typeid (proc) == typeid (OutputProcessor);
    if (! isIOProcessor)
    {
        const auto xButtonSize = proportionOfWidth (0.1f);
        const auto xButtonPad = proportionOfWidth (0.015f);
        powerButton.setBounds (width - 2 * xButtonSize, 0, xButtonSize, xButtonSize);
        xButton.setBounds (Rectangle { width - xButtonSize, 0, xButtonSize, xButtonSize }.reduced (xButtonPad));
    }

    const int portDim = proportionOfHeight (0.15f);
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

void ProcessorEditor::mouseDown (const MouseEvent& e)
{
    mouseDownOffset = e.getEventRelativeTo (this).getPosition();
}

void ProcessorEditor::mouseDrag (const MouseEvent& e)
{
    const auto relE = e.getEventRelativeTo (getParentComponent());

    if (auto* parent = getParentComponent())
    {
        auto parentBounds = parent->getBounds();
        proc.setPosition (relE.getPosition() - mouseDownOffset, parentBounds);
        setTopLeftPosition (proc.getPosition (parentBounds));
        parent->repaint();
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

void ProcessorEditor::setConnectionStatus (bool isConnected, int portIndex, bool isInput)
{
    if (isInput)
        inputPorts[portIndex]->setConnectionStatus (isConnected);
    else
        outputPorts[portIndex]->setConnectionStatus (isConnected);
}
