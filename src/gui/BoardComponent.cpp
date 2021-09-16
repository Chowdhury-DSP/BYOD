#include "BoardComponent.h"

namespace
{
constexpr int editorWidth = 330;
constexpr int editorHeight = 220;
constexpr int editorPad = 10;
constexpr int newButtonWidth = 40;
constexpr int newButtonPad = 10;
} // namespace

BoardComponent::BoardComponent (ProcessorChain& procs) : procChain (procs)
{
    newProcButton.setButtonText ("+");
    newProcButton.setColour (TextButton::buttonColourId, Colours::black.withAlpha (0.65f));
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
}

BoardComponent::~BoardComponent()
{
    inputEditor->removePortListener (this);
    outputEditor->removePortListener (this);
    procChain.removeListener (this);
}

int BoardComponent::getIdealWidth (int parentWidth) const
{
    int idealWidth = processorEditors.size() * (editorWidth + 2 * editorPad) + newButtonWidth + 2 * newButtonPad;
    return jmax (idealWidth, parentWidth < 0 ? getParentWidth() : parentWidth);
}

void BoardComponent::paint (Graphics& g)
{
    g.setColour (Colours::red);
    for (auto* cable : cables)
    {
        auto* startEditor = findEditorForProcessor (cable->startProc);
        auto startPortLocation = startEditor->getPortLocation (cable->startIdx, false);
        startPortLocation += startEditor->getBounds().getTopLeft();

        if (cable->endProc != nullptr)
        {
            auto* endEditor = findEditorForProcessor (cable->endProc);
            auto endPortLocation = endEditor->getPortLocation (cable->endIdx, true);
            endPortLocation += endEditor->getBounds().getTopLeft();

            auto cableLine = Line (startPortLocation.toFloat(), endPortLocation.toFloat());
            g.drawLine (cableLine, 5.0f);
        }
        else if (cableMouse != nullptr)
        {
            auto cableLine = Line (startPortLocation.toFloat(), cableMouse->getPosition().toFloat());
            g.drawLine (cableLine, 5.0f);
        }
    }
}

void BoardComponent::resized()
{
    auto centreEditorHeight = (getHeight() - editorHeight) / 2;
    inputEditor->setBounds (editorPad, centreEditorHeight, editorWidth / 2, editorHeight);
    outputEditor->setBounds (getWidth() - (editorWidth / 2 + editorPad), centreEditorHeight, editorWidth / 2, editorHeight);

    newProcButton.setBounds (getWidth() - newButtonWidth, 0, newButtonWidth, newButtonWidth);
    infoComp.setBounds (Rectangle<int> (jmin (400, getWidth()), jmin (250, getHeight())).withCentre (getLocalBounds().getCentre()));
}

void BoardComponent::refreshBoardSize()
{
    auto newWidth = getIdealWidth();
    auto oldWidth = getWidth();
    setSize (newWidth, getHeight());

    if (newWidth == oldWidth)
        resized();
}

void BoardComponent::processorAdded (BaseProcessor* newProc)
{
    auto* newEditor = processorEditors.add (std::make_unique<ProcessorEditor> (*newProc, procChain, this));
    addAndMakeVisible (newEditor);

    auto centre = getLocalBounds().getCentre();
    newEditor->setBounds (Rectangle (editorWidth, editorHeight).withCentre (centre));

    refreshBoardSize();

    newEditor->addPortListener (this);
}

void BoardComponent::processorRemoved (const BaseProcessor* proc)
{
    auto* editor = findEditorForProcessor (proc);
    editor->removePortListener (this);
    processorEditors.removeObject (editor);

    refreshBoardSize();
}

void BoardComponent::processorMoved (int procToMove, int procInSlot)
{
    processorEditors.move (procToMove, procInSlot);
    resized();
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

    auto tryToConnectToEditor = [=] (ProcessorEditor* editor) -> bool
    {
        int numPorts = editor->getProcPtr()->getNumInputs();
        for (int i = 0; i < numPorts; ++i)
        {
            auto portLocation = editor->getPortLocation (i, true);
            portLocation += editor->getBounds().getTopLeft();
            auto distanceFromPort = mousePos.getDistanceFrom (portLocation);
            if (distanceFromPort < 25)
            {
                auto* cable = cables.getLast();
                auto* endProc = editor->getProcPtr();

                cable->startProc->addOutputProcessor (endProc, i);

                cable->endProc = editor->getProcPtr();
                cable->endIdx = i;
                repaint();
                return true;
            }
        }

        return false;
    };

    for (auto* editor : processorEditors)
    {
        if (tryToConnectToEditor (editor))
            return;
    }

    if (tryToConnectToEditor (outputEditor.get()))
        return;

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
            cable->startProc->removeOutputProcessor (cable->endProc, cable->startIdx);
            cables.removeObject (cable);
            break;
        }
    }

    repaint();
}
