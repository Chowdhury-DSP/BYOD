#include "BoardComponent.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
constexpr int editorWidth = 270;
constexpr int editorHeight = 180;
constexpr int editorPad = 10;
constexpr int newButtonWidth = 40;

constexpr int getScaleDim (int dim, float scaleFactor)
{
    return int ((float) dim * scaleFactor);
}
} // namespace

BoardComponent::BoardComponent (ProcessorChain& procs) : procChain (procs), cableView (this)
{
    newProcButton.setButtonText ("+");
    newProcButton.setColour (TextButton::buttonColourId, Colours::azure.darker (0.8f).withAlpha (0.75f));
    newProcButton.setColour (ComboBox::outlineColourId, Colours::white);
    addAndMakeVisible (newProcButton);
    newProcButton.onClick = [=]
    { showNewProcMenu(); };

    inputEditor = std::make_unique<ProcessorEditor> (procs.getInputProcessor(), procChain, this);
    addAndMakeVisible (inputEditor.get());
    inputEditor->addPortListener (&cableView);

    outputEditor = std::make_unique<ProcessorEditor> (procs.getOutputProcessor(), procChain, this);
    addAndMakeVisible (outputEditor.get());
    outputEditor->addPortListener (&cableView);

    setSize (800, 800);

    addChildComponent (infoComp);
    addAndMakeVisible (cableView);
    cableView.toBack();

    for (auto* p : procs.getProcessors())
        processorAdded (p);

    procChain.addListener (this);
    procChain.addListener (&cableView);

    cableView.refreshConnections();
}

BoardComponent::~BoardComponent()
{
    inputEditor->removePortListener (&cableView);
    outputEditor->removePortListener (&cableView);
    procChain.removeListener (this);
    procChain.removeListener (&cableView);
}

void BoardComponent::setScaleFactor (float newScaleFactor)
{
    scaleFactor = newScaleFactor;
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
    inputEditor->setBounds (editorPad, centreEditorHeight, thisEditorWidth / 2, thisEditorHeight);
    outputEditor->setBounds (width - (thisEditorWidth / 3 + editorPad), centreEditorHeight, thisEditorWidth / 3, thisEditorHeight);

    for (auto* editor : processorEditors)
        setEditorPosition (editor);

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

    auto* newEditor = processorEditors.add (std::make_unique<ProcessorEditor> (*newProc, procChain, this));
    addAndMakeVisible (newEditor);

    cableView.processorBeingAdded (newProc);
    setEditorPosition (newEditor);

    newEditor->addPortListener (&cableView);

    repaint();
}

void BoardComponent::processorRemoved (const BaseProcessor* proc)
{
    cableView.processorBeingRemoved (proc);

    if (auto* editor = findEditorForProcessor (proc))
    {
        editor->removePortListener (&cableView);
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

    if (inputEditor != nullptr && inputEditor->getProcPtr() == proc)
        return inputEditor.get();

    if (outputEditor != nullptr && outputEditor->getProcPtr() == proc)
        return outputEditor.get();

    return nullptr;
}

void BoardComponent::setEditorPosition (ProcessorEditor* editor)
{
    const auto thisEditorWidth = getScaleDim (editorWidth, scaleFactor);
    const auto thisEditorHeight = getScaleDim (editorHeight, scaleFactor);

    auto* proc = editor->getProcPtr();
    auto position = proc->getPosition (getBounds());
    if (position == Point (0, 0) && getWidth() > 0 && getHeight() > 0) // no position set yet
    {
        auto b = getLocalBounds()
                     .withWidth (getWidth() * 2 / 3)
                     .withHeight (getHeight() * 2 / 3);

        auto randX = jmax (proportionOfWidth (0.1f), 1);
        auto randY = jmax (proportionOfHeight (0.1f), 1);
        auto& rand = Random::getSystemRandom();
        auto centre = b.getCentre() + Point (rand.nextInt ({ -randX, randX }), rand.nextInt ({ -randY, randY }));

        editor->setBounds (Rectangle (thisEditorWidth, thisEditorHeight).withCentre (centre));
        proc->setPosition (editor->getBounds().getTopLeft(), getBounds());
    }
    else
    {
        editor->setBounds (Rectangle (thisEditorWidth, thisEditorHeight).withPosition (position));
    }
}
