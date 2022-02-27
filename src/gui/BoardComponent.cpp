#include "BoardComponent.h"
#include "cables/CableViewConnectionHelper.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
constexpr int editorWidth = 270;
constexpr int editorHeight = 180;
constexpr int editorPad = 5;

constexpr int getScaleDim (int dim, float scaleFactor)
{
    return int ((float) dim * scaleFactor);
}
} // namespace

BoardComponent::BoardComponent (ProcessorChain& procs) : procChain (procs), cableView (this)
{
    inputEditor = std::make_unique<ProcessorEditor> (procs.getInputProcessor(), procChain);
    addAndMakeVisible (inputEditor.get());
    inputEditor->addListener (this);

    outputEditor = std::make_unique<ProcessorEditor> (procs.getOutputProcessor(), procChain);
    addAndMakeVisible (outputEditor.get());
    outputEditor->addListener (this);

    addChildComponent (infoComp);
    addAndMakeVisible (cableView);
    cableView.toBack();
    addMouseListener (&cableView, true);

    for (auto* p : procs.getProcessors())
        processorAdded (p);

    procChain.addListener (this);
    procChain.addListener (cableView.getConnectionHelper());

    cableView.getConnectionHelper()->refreshConnections();

    popupMenu.setAssociatedComponent (this);
    popupMenu.popupMenuCallback = [&] (PopupMenu& menu, PopupMenu::Options& options)
    { showNewProcMenu (menu, options); };
}

BoardComponent::~BoardComponent()
{
    inputEditor->removeListener (this);
    outputEditor->removeListener (this);

    removeMouseListener (&cableView);
    procChain.removeListener (this);
    procChain.removeListener (cableView.getConnectionHelper());
}

void BoardComponent::setScaleFactor (float newScaleFactor)
{
    scaleFactor = newScaleFactor;
    cableView.setScaleFactor (scaleFactor);

    resized();
}

void BoardComponent::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    cableView.setBounds (getLocalBounds());

    const auto thisEditorWidth = getScaleDim (editorWidth, scaleFactor);
    const auto thisEditorHeight = getScaleDim (editorHeight, scaleFactor);
    auto centreEditorHeight = (height - thisEditorHeight) / 2;

    setEditorPosition (inputEditor.get(), Rectangle (editorPad, centreEditorHeight, thisEditorWidth / 2, thisEditorHeight));
    setEditorPosition (outputEditor.get(), Rectangle (width - (thisEditorWidth / 2 + editorPad), centreEditorHeight, thisEditorWidth / 2, thisEditorHeight));

    for (auto* editor : processorEditors)
        setEditorPosition (editor);

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

    auto* newEditor = processorEditors.add (std::make_unique<ProcessorEditor> (*newProc, procChain));
    addAndMakeVisible (newEditor);

    cableView.processorBeingAdded (newProc);
    setEditorPosition (newEditor);

    newEditor->addListener (this);

    repaint();
}

void BoardComponent::processorRemoved (const BaseProcessor* proc)
{
    cableView.processorBeingRemoved (proc);

    if (auto* editor = findEditorForProcessor (proc))
    {
        editor->removeListener (this);
        processorEditors.removeObject (editor);
    }

    repaint();
}

void BoardComponent::showInfoComp (const BaseProcessor& proc)
{
    infoComp.setInfoForProc (proc.getName(), proc.getUIOptions().info);
    infoComp.setVisible (true);
    infoComp.toFront (true);
}

void BoardComponent::editorDragged (ProcessorEditor& editor, const MouseEvent& e, const Point<int>& mouseOffset)
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

void BoardComponent::showNewProcMenu (PopupMenu& menu, PopupMenu::Options& options)
{
    nextEditorPosition = options.getTargetScreenArea().getPosition() - getScreenPosition();

    int menuID = 0;
    procChain.getProcStore().createProcList (menu, menuID);

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
    if (position == Point (0, 0) && getWidth() > 0 && getHeight() > 0) // no position set yet
    {
        if (bounds == Rectangle<int> {})
            bounds = Rectangle (thisEditorWidth, thisEditorHeight).withCentre (nextEditorPosition);

        editor->setBounds (bounds);
        proc->setPosition (editor->getBounds().getTopLeft(), getBounds());
    }
    else
    {
        if (bounds == Rectangle<int> {})
            bounds = Rectangle (thisEditorWidth, thisEditorHeight);

        editor->setBounds (bounds.withPosition (position));
    }
}
