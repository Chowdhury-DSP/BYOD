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
    { procChain.getActionHelper().removeProcessor (&proc); };

    if (proc.getNumInputs() != 0 && proc.getNumOutputs() != 0)
    {
        auto cog = Drawable::createFromImageData (BinaryData::cogsolid_svg, BinaryData::cogsolid_svgSize);
        cog->replaceColour (Colours::white, contrastColour);
        settingsButton.setImages (cog.get());
        addAndMakeVisible (settingsButton);

        settingsButton.onClick = [&]
        {
            PopupMenu menu;
            PopupMenu::Options options;
            processorSettingsCallback (menu, options);
            menu.showMenuAsync (options);
        };
    }

    if (auto* lnf = proc.getCustomLookAndFeel())
        setLookAndFeel (lnf);
    else
        setLookAndFeel (lnfAllocator->getLookAndFeel<ProcessorLNF>());

    for (int i = 0; i < baseProc.getNumInputs(); ++i)
    {
        const auto portType = i == baseProc.getInputModulationPort() ? modulation : audio;
        auto newPort = inputPorts.add (std::make_unique<Port> (procUI.backgroundColour, portType));
        newPort->setInputOutput (true);
        addAndMakeVisible (newPort);
    }

    for (int i = 0; i < baseProc.getNumOutputs(); ++i)
    {
        const auto portType = i == baseProc.getOutputModulationPort() ? modulation : audio;
        auto newPort = outputPorts.add (std::make_unique<Port> (procUI.backgroundColour, portType));
        newPort->setInputOutput (false);
        addAndMakeVisible (newPort);
    }
}

ProcessorEditor::~ProcessorEditor() = default;

void ProcessorEditor::addToBoard (BoardComponent* boardComp)
{
    broadcasterCallbacks += {
        showInfoCompBroadcaster.connect<&BoardComponent::showInfoComp> (boardComp),
        editorDraggedBroadcaster.connect<&BoardComponent::editorDragged> (boardComp),
        duplicateProcessorBroadcaster.connect<&BoardComponent::duplicateProcessor> (boardComp),
    };
}

void ProcessorEditor::processorSettingsCallback (PopupMenu& menu, PopupMenu::Options& options)
{
    proc.addToPopupMenu (menu);

    menu.addItem ("Reset", [&]
                  { resetProcParameters(); });

    PopupMenu replaceProcMenu;
    createReplaceProcMenu (replaceProcMenu);
    if (replaceProcMenu.containsAnyActiveItems())
        menu.addSubMenu ("Replace", replaceProcMenu);

    menu.addItem ("Duplicate", [&]
                  { duplicateProcessorBroadcaster (*this); });

    menu.addItem ("Info", [this]
                  { showInfoCompBroadcaster (proc); });

    menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ProcessorLNF>());
    options = options
                  .withParentComponent (getParentComponent())
                  .withStandardItemHeight (27);
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
    int menuID = 100;
    procChain.getProcStore().createProcReplaceList (menu, menuID, &proc);
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
        settingsButton.setBounds (Rectangle { width - 3 * xButtonSize, 0, xButtonSize, xButtonSize }.reduced (proportionOfWidth (0.01f)));
        powerButton.setBounds (width - 2 * xButtonSize, 0, xButtonSize, xButtonSize);
        xButton.setBounds (Rectangle { width - xButtonSize, 0, xButtonSize, xButtonSize }.reduced (proportionOfWidth (0.015f)));
    }

    const int portDim = proportionOfHeight (0.17f);
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
    editorDraggedBroadcaster (*this, e, mouseDownOffset);
}

Port* ProcessorEditor::getPortPrivate (int portIndex, bool isInput) const
{
    if (isInput)
    {
        jassert (portIndex < inputPorts.size());
        return inputPorts[portIndex];
    }

    jassert (portIndex < outputPorts.size());
    return outputPorts[portIndex];
}

Port* ProcessorEditor::getPort (int portIndex, bool isInput)
{
    return getPortPrivate (portIndex, isInput);
}

juce::Point<int> ProcessorEditor::getPortLocation (int portIndex, bool isInput) const
{
    return getPortPrivate (portIndex, isInput)->getBounds().getCentre();
}

void ProcessorEditor::setConnectionStatus (bool isConnected, int portIndex, bool isInput)
{
    if (isInput)
        inputPorts[portIndex]->setConnectionStatus (isConnected);
    else
        outputPorts[portIndex]->setConnectionStatus (isConnected);
}
