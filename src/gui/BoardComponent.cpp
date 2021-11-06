#include "BoardComponent.h"

namespace
{
constexpr int editorWidth = 270;
constexpr int editorHeight = 180;
constexpr int editorPad = 10;
constexpr int newButtonWidth = 40;
constexpr int portDistanceLimit = 25;

Point<int> getPortLocation (ProcessorEditor* editor, int portIdx, bool isInput)
{
    auto portLocation = editor->getPortLocation (portIdx, isInput);
    return portLocation + editor->getBounds().getTopLeft();
}

ConnectionInfo cableToConnection (const Cable& cable)
{
    return { cable.startProc, cable.startIdx, cable.endProc, cable.endIdx };
}

std::unique_ptr<Cable> connectionToCable (const ConnectionInfo& connection)
{
    return std::make_unique<Cable> (connection.startProc, connection.startPort, connection.endProc, connection.endPort);
}

void addConnectionsForProcessor (OwnedArray<Cable>& cables, BaseProcessor* proc)
{
    for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
    {
        auto numConnections = proc->getNumOutputConnections (portIdx);
        for (int cIdx = 0; cIdx < numConnections; ++cIdx)
        {
            const auto& connection = proc->getOutputConnection (portIdx, cIdx);
            cables.add (connectionToCable (connection));
        }
    }
}
} // namespace

BoardComponent::BoardComponent (ProcessorChain& procs) : procChain (procs)
{
    newProcButton.setButtonText ("+");
    newProcButton.setColour (TextButton::buttonColourId, Colours::azure.darker (0.8f).withAlpha (0.75f));
    newProcButton.setColour (ComboBox::outlineColourId, Colours::white);
    addAndMakeVisible (newProcButton);
    newProcButton.onClick = [=]
    { showNewProcMenu(); };

    inputEditor = std::make_unique<ProcessorEditor> (procs.getInputProcessor(), procChain, this);
    addAndMakeVisible (inputEditor.get());
    inputEditor->addPortListener (this);

    outputEditor = std::make_unique<ProcessorEditor> (procs.getOutputProcessor(), procChain, this);
    addAndMakeVisible (outputEditor.get());
    outputEditor->addPortListener (this);

    setSize (800, 800);

    addChildComponent (infoComp);

    for (auto* p : procs.getProcessors())
        processorAdded (p);

    procChain.addListener (this);

    refreshConnections();
}

BoardComponent::~BoardComponent()
{
    inputEditor->removePortListener (this);
    outputEditor->removePortListener (this);
    procChain.removeListener (this);
}

void BoardComponent::paint (Graphics& g)
{
    const Colour baseColour = Colour (0xFFD0592C);
    const float cableThickness = 5.0f;

    g.setColour (baseColour.brighter (0.1f));
    for (auto* cable : cables)
    {
        auto* startEditor = findEditorForProcessor (cable->startProc);
        auto startPortLocation = getPortLocation (startEditor, cable->startIdx, false);

        if (cable->endProc != nullptr)
        {
            auto* endEditor = findEditorForProcessor (cable->endProc);
            auto endPortLocation = getPortLocation (endEditor, cable->endIdx, true);

            auto cableLine = Line (startPortLocation.toFloat(), endPortLocation.toFloat());
            g.drawLine (cableLine, cableThickness);
        }
        else if (cableMouse != nullptr)
        {
            auto mousePos = cableMouse->getPosition();
            auto [editor, portIdx] = getNearestInputPort (mousePos);
            if (editor != nullptr)
            {
                Graphics::ScopedSaveState graphicsState (g);
                g.setColour (baseColour.darker (0.1f));
                g.setOpacity (0.5f);

                auto endPortLocation = getPortLocation (editor, portIdx, true);
                auto glowBounds = (Rectangle (portDistanceLimit, portDistanceLimit) * 2).withCentre (endPortLocation);
                g.fillEllipse (glowBounds.toFloat());
            }

            auto cableLine = Line (startPortLocation.toFloat(), mousePos.toFloat());
            g.drawLine (cableLine, cableThickness);
        }
    }
}

void BoardComponent::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    auto centreEditorHeight = (height - editorHeight) / 2;
    inputEditor->setBounds (editorPad, centreEditorHeight, editorWidth / 2, editorHeight);
    outputEditor->setBounds (width - (editorWidth / 3 + editorPad), centreEditorHeight, editorWidth / 3, editorHeight);

    for (auto* editor : processorEditors)
        setEditorPosition (editor);

    newProcButton.setBounds (width - newButtonWidth, 0, newButtonWidth, newButtonWidth);
    infoComp.setBounds (Rectangle<int> (jmin (400, width), jmin (250, height)).withCentre (getLocalBounds().getCentre()));
}

void BoardComponent::processorAdded (BaseProcessor* newProc)
{
    auto* newEditor = processorEditors.add (std::make_unique<ProcessorEditor> (*newProc, procChain, this));
    addAndMakeVisible (newEditor);

    addConnectionsForProcessor (cables, newProc);
    setEditorPosition (newEditor);

    newEditor->addPortListener (this);

    repaint();
}

void BoardComponent::processorRemoved (const BaseProcessor* proc)
{
    for (int i = cables.size() - 1; i >= 0; --i)
    {
        if (cables[i]->startProc == proc || cables[i]->endProc == proc)
            cables.remove (i);
    }

    auto* editor = findEditorForProcessor (proc);
    editor->removePortListener (this);
    processorEditors.removeObject (editor);

    repaint();
}

void BoardComponent::refreshConnections()
{
    cables.clear();

    for (auto* proc : procChain.getProcessors())
        addConnectionsForProcessor (cables, proc);

    addConnectionsForProcessor (cables, &procChain.getInputProcessor());

    repaint();
}

void BoardComponent::connectionAdded (const ConnectionInfo& info)
{
    if (ignoreConnectionCallbacks)
        return;

    cables.add (connectionToCable (info));
    repaint();
}

void BoardComponent::connectionRemoved (const ConnectionInfo& info)
{
    if (ignoreConnectionCallbacks)
        return;

    for (auto* cable : cables)
    {
        if (cable->startProc == info.startProc
            && cable->startIdx == info.startPort
            && cable->endProc == info.endProc
            && cable->endIdx == info.endPort)
        {
            cables.removeObject (cable);
            break;
        }
    }

    repaint();
}

void BoardComponent::showInfoComp (const BaseProcessor& proc)
{
    infoComp.setInfoForProc (proc.getName(), proc.getUIOptions().info);
    infoComp.setVisible (true);
    infoComp.toFront (true);
}

void BoardComponent::showNewProcMenu() const
{
    auto& procStore = procChain.getProcStore();

    int menuID = 0;
    PopupMenu menu;
    for (auto type : { Drive, Tone, Utility, Other })
    {
        PopupMenu subMenu;
        procStore.createProcList (subMenu, menuID, type);

        auto typeName = std::string (magic_enum::enum_name (type));
        menu.addSubMenu (String (typeName), subMenu);
    }

    auto options = PopupMenu::Options()
                       .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards)
                       .withMinimumWidth (125)
                       .withStandardItemHeight (27);

    menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ByodLNF>());
    menu.showMenuAsync (options);
}

ProcessorEditor* BoardComponent::findEditorForProcessor (const BaseProcessor* proc) const
{
    for (auto* editor : processorEditors)
        if (editor->getProcPtr() == proc)
            return editor;

    if (inputEditor->getProcPtr() == proc)
        return inputEditor.get();

    if (outputEditor->getProcPtr() == proc)
        return outputEditor.get();

    jassertfalse;
    return nullptr;
}

void BoardComponent::createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e)
{
    cables.add (std::make_unique<Cable> (origin->getProcPtr(), portIndex));
    cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (this)));
    repaint();
}

void BoardComponent::refreshCable (const MouseEvent& e)
{
    cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (this)));
    repaint();
}

void BoardComponent::releaseCable (const MouseEvent& e)
{
    cableMouse.reset();

    // check if we're releasing near an output port
    auto relMouse = e.getEventRelativeTo (this);
    auto mousePos = relMouse.getPosition();

    auto [editor, portIdx] = getNearestInputPort (mousePos);
    if (editor != nullptr)
    {
        auto* cable = cables.getLast();
        cable->endProc = editor->getProcPtr();
        cable->endIdx = portIdx;

        const ScopedValueSetter<bool> svs (ignoreConnectionCallbacks, true);
        auto connection = cableToConnection (*cable);
        procChain.addConnection (std::move (connection));

        repaint();
        return;
    }

    // not being connected... trash the latest cable
    cables.removeObject (cables.getLast());

    repaint();
}

void BoardComponent::destroyCable (ProcessorEditor* origin, int portIndex)
{
    const auto* proc = origin->getProcPtr();
    for (auto* cable : cables)
    {
        if (cable->endProc == proc && cable->endIdx == portIndex)
        {
            const ScopedValueSetter<bool> svs (ignoreConnectionCallbacks, true);
            procChain.removeConnection (cableToConnection (*cable));
            cables.removeObject (cable);

            break;
        }
    }

    repaint();
}

std::pair<ProcessorEditor*, int> BoardComponent::getNearestInputPort (const Point<int>& pos) const
{
    auto result = std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    int minDistance = -1;

    auto checkPorts = [&] (ProcessorEditor* editor)
    {
        int numPorts = editor->getProcPtr()->getNumInputs();
        for (int i = 0; i < numPorts; ++i)
        {
            auto portLocation = getPortLocation (editor, i, true);
            auto distanceFromPort = pos.getDistanceFrom (portLocation);

            bool isClosest = (distanceFromPort < portDistanceLimit && minDistance < 0) || distanceFromPort < minDistance;
            if (isClosest)
            {
                minDistance = distanceFromPort;
                result = std::make_pair (editor, i);
            }
        }
    };

    for (auto* editor : processorEditors)
        checkPorts (editor);

    checkPorts (outputEditor.get());

    if (result.first == nullptr)
        return result;

    for (auto* cable : cables)
    {
        // the closest port is already connected!
        if (cable->endProc == result.first->getProcPtr() && cable->endIdx == result.second)
            return std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    }

    return result;
}

void BoardComponent::setEditorPosition (ProcessorEditor* editor)
{
    auto* proc = editor->getProcPtr();
    auto position = proc->getPosition (getBounds());
    if (position == Point (0, 0)) // no position set yet
    {
        auto b = getLocalBounds()
                     .withWidth (getWidth() * 2 / 3)
                     .withHeight (getHeight() * 2 / 3);

        auto randX = proportionOfWidth (0.1f);
        auto randY = proportionOfHeight (0.1f);
        auto& rand = Random::getSystemRandom();
        auto centre = b.getCentre() + Point (rand.nextInt ({ -randX, randX }), rand.nextInt ({ -randY, randY }));

        editor->setBounds (Rectangle (editorWidth, editorHeight).withCentre (centre));
        proc->setPosition (editor->getBounds().getTopLeft(), getBounds());
    }
    else
    {
        editor->setBounds (Rectangle (editorWidth, editorHeight).withPosition (position));
    }
}
