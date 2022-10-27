#include "BoardComponent.h"
#include "cables/CableViewConnectionHelper.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
constexpr int editorWidth = 270;
constexpr int editorHeight = 180;
constexpr int editorPad = 5;
constexpr int newButtonWidth = 40;

constexpr int getScaleDim (int dim, float scaleFactor)
{
    return int ((float) dim * scaleFactor);
}

juce::Point<int> getRandomPosition (const Component& comp)
{
    auto b = comp.getLocalBounds()
                 .withWidth (comp.getWidth() * 2 / 3)
                 .withHeight (comp.getHeight() * 2 / 3);

    auto randX = jmax (comp.proportionOfWidth (0.1f), 1);
    auto randY = jmax (comp.proportionOfHeight (0.1f), 1);
    auto& rand = Random::getSystemRandom();
    return b.getCentre() + juce::Point (rand.nextInt ({ -randX, randX }), rand.nextInt ({ -randY, randY }));
}
} // namespace

BoardComponent::BoardComponent (ProcessorChain& procs, const HostContextProvider& hostCP) : Component ("Board"),
                                                                                            procChain (procs),
                                                                                            cableView (this),
                                                                                            hostContextProvider (hostCP)
{
    newProcButton.setButtonText ("+");
    newProcButton.setColour (TextButton::buttonColourId, Colours::azure.darker (0.8f).withAlpha (0.75f));
    newProcButton.setColour (ComboBox::outlineColourId, Colours::white);
    addAndMakeVisible (newProcButton);
    newProcButton.onClick = [&]
    {
        ScopedValueSetter svs { addingFromNewProcButton, true };
        popupMenu.showPopupMenu();
    };

    inputEditor = std::make_unique<ProcessorEditor> (procs.getInputProcessor(), procChain, hostContextProvider);
    addAndMakeVisible (inputEditor.get());
    inputEditor->addToBoard (this);

    outputEditor = std::make_unique<ProcessorEditor> (procs.getOutputProcessor(), procChain, hostContextProvider);
    addAndMakeVisible (outputEditor.get());
    outputEditor->addToBoard (this);

    addChildComponent (infoComp);
    addAndMakeVisible (cableView);
    cableView.toBack();
    addMouseListener (&cableView, true);

    for (auto* p : procs.getProcessors())
        processorAdded (p);

    callbacks += {
        procChain.processorAddedBroadcaster.connect<&BoardComponent::processorAdded> (this),
        procChain.processorRemovedBroadcaster.connect<&BoardComponent::processorRemoved> (this),
        procChain.refreshConnectionsBroadcaster.connect<&BoardComponent::refreshConnections> (this),
        procChain.connectionAddedBroadcaster.connect<&BoardComponent::connectionAdded> (this),
        procChain.connectionRemovedBroadcaster.connect<&BoardComponent::connectionRemoved> (this),
    };

    cableView.getConnectionHelper()->connectToProcessorChain (procChain);

    popupMenu.setAssociatedComponent (this);
    popupMenu.popupMenuCallback = [&] (PopupMenu& menu, PopupMenu::Options& options)
    {
        menu.addSectionHeader ("Add Processor:");
        menu.addSeparator();
        showNewProcMenu (menu, options);
    };
}

BoardComponent::~BoardComponent()
{
    removeMouseListener (&cableView);
}

void BoardComponent::setScaleFactor (float newScaleFactor)
{
    scaleFactor = newScaleFactor;

    resized();
}

float BoardComponent::getScaleFactor() const
{
    return scaleFactor;
}

void BoardComponent::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();


    const auto thisEditorWidth = getScaleDim (editorWidth, scaleFactor);
    const auto thisEditorHeight = getScaleDim (editorHeight, scaleFactor);
    auto centreEditorHeight = (height - thisEditorHeight) / 2;

    setEditorPosition (inputEditor.get(), Rectangle (editorPad, centreEditorHeight, thisEditorWidth / 2, thisEditorHeight));
    setEditorPosition (outputEditor.get(), Rectangle (width - (thisEditorWidth / 2 + editorPad), centreEditorHeight, thisEditorWidth / 2, thisEditorHeight));

    for (auto* editor : processorEditors)
        setEditorPosition (editor);
    
    cableView.setBounds (getLocalBounds());

    newProcButton.setBounds (width - newButtonWidth, 0, newButtonWidth, newButtonWidth);
    infoComp.setBounds (Rectangle<int> (jmin (400, width), jmin (250, height)).withCentre (getLocalBounds().getCentre()));

    repaint();
}

void BoardComponent::processorAdded (BaseProcessor* newProc)
{
    if (! procChain.getProcessors().contains (newProc))
    {
        jassertfalse; // being asked to add a processor that is no longer in the chain?
        return;
    }

    auto* newEditor = processorEditors.add (std::make_unique<ProcessorEditor> (*newProc, procChain, hostContextProvider));
    addAndMakeVisible (newEditor);

    cableView.processorBeingAdded (newProc);
    setEditorPosition (newEditor);

    newEditor->addToBoard (this);

    repaint();
}

void BoardComponent::processorRemoved (const BaseProcessor* proc)
{
    cableView.processorBeingRemoved (proc);

    if (auto* editor = findEditorForProcessor (proc))
    {
        processorEditors.removeObject (editor);
    }
    repaint();
}

void BoardComponent::connectionAdded (const ConnectionInfo& info) const
{
    if (auto* editor = findEditorForProcessor (info.endProc))
        editor->toggleParamsEnabledOnInputConnectionChange (info.endPort, true);
}

void BoardComponent::connectionRemoved (const ConnectionInfo& info) const
{
    if (auto* editor = findEditorForProcessor (info.endProc))
        editor->toggleParamsEnabledOnInputConnectionChange (info.endPort, false);
}

void BoardComponent::showInfoComp (const BaseProcessor& proc)
{
    infoComp.setInfoForProc (proc.getName(), proc.getUIOptions().info);
    infoComp.setVisible (true);
    infoComp.toFront (true);
}

void BoardComponent::editorDragged (ProcessorEditor& editor, const MouseEvent& e, const juce::Point<int>& mouseOffset)
{
    const auto relE = e.getEventRelativeTo (this);
    const auto bounds = getBounds();

    auto* proc = editor.getProcPtr();
    proc->setPosition (relE.getPosition() - mouseOffset, bounds);
    editor.setTopLeftPosition (proc->getPosition (bounds));
    repaint();
}

void BoardComponent::duplicateProcessor (const ProcessorEditor& editor)
{
    const auto xOff = proportionOfWidth (0.075f);
    const auto yOff = proportionOfHeight (0.075f);
    nextEditorPosition = editor.getBoundsInParent().getCentre().translated (xOff, yOff);

    procChain.getProcStore().duplicateProcessor (*editor.getProcPtr());
}

void BoardComponent::showNewProcMenu (PopupMenu& menu, PopupMenu::Options& options, ConnectionInfo* connectionInfo)
{
    if (addingFromNewProcButton)
    {
        nextEditorPosition = getRandomPosition (*this);
    }
    else
    {
        nextEditorPosition = options.getTargetScreenArea().getPosition() - getScreenPosition();

        const auto halfEditorWidth = getScaleDim (editorWidth, scaleFactor) / 2;
        const auto halfEditorHeight = getScaleDim (editorHeight, scaleFactor) / 2;
        nextEditorPosition.x = jlimit (halfEditorWidth, getWidth() - halfEditorWidth, nextEditorPosition.x);
        nextEditorPosition.y = jlimit (halfEditorHeight, getHeight() - halfEditorHeight, nextEditorPosition.y);
    }

    int menuID = 0;

    if (connectionInfo != nullptr)
    {
        procChain.getProcStore().createProcFromCableClickList (menu, menuID, connectionInfo);
    }
    else
    {
        procChain.getProcStore().createProcList (menu, menuID);
    }

    options = options
                  .withParentComponent (this)
                  .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards)
                  .withMinimumWidth (125)
                  .withStandardItemHeight (27);

    menu.setLookAndFeel (lnfAllocator->getLookAndFeel<ByodLNF>());
}

ProcessorEditor* BoardComponent::findEditorForProcessor (const BaseProcessor* proc) const
{
    for (auto* editor : processorEditors)
        if (editor->getProcPtr() == proc)
            return editor;

    if (inputEditor != nullptr && inputEditor->getProcPtr() == proc)
        return inputEditor.get();

    if (outputEditor != nullptr && outputEditor->getProcPtr() == proc)
        return outputEditor.get();

    return nullptr;
}

void BoardComponent::setEditorPosition (ProcessorEditor* editor, Rectangle<int> bounds)
{
    const auto thisEditorWidth = getScaleDim (editorWidth, scaleFactor);
    const auto thisEditorHeight = getScaleDim (editorHeight, scaleFactor);

    auto* proc = editor->getProcPtr();
    auto position = proc->getPosition (getBounds());
    if (position == juce::Point (0, 0) && getWidth() > 0 && getHeight() > 0) // no position set yet
    {
        if (bounds == Rectangle<int> {})
            bounds = Rectangle (thisEditorWidth, thisEditorHeight).withCentre (nextEditorPosition);

        editor->setBounds (bounds);
        proc->setPosition (editor->getBounds().getTopLeft(), getBounds());
        editor->setTopLeftPosition (proc->getPosition (getLocalBounds()));
    }
    else
    {
        if (bounds == Rectangle<int> {})
            bounds = Rectangle (thisEditorWidth, thisEditorHeight);

        editor->setBounds (bounds.withPosition (position));
    }
}
