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

    setSize (800, 800);

    addChildComponent (infoComp);

    for (auto* p : procs.getProcessors())
        processorAdded (p);

    procChain.addListener (this);

    addAndMakeVisible (inputEditor);
    addAndMakeVisible (outputEditor);
}

BoardComponent::~BoardComponent()
{
    procChain.removeListener (this);
}

int BoardComponent::getIdealWidth (int parentWidth) const
{
    int idealWidth = processorEditors.size() * (editorWidth + 2 * editorPad) + newButtonWidth + 2 * newButtonPad;
    return jmax (idealWidth, parentWidth < 0 ? getParentWidth() : parentWidth);
}

void BoardComponent::paint (Graphics& g)
{
    ignoreUnused (g);
}

void BoardComponent::resized()
{
    auto centreEditorHeight = (getHeight() - editorHeight) / 2;
    inputEditor.setBounds (editorPad, centreEditorHeight, editorWidth / 2, editorHeight);
    outputEditor.setBounds (getWidth() - (editorWidth / 2 + editorPad), centreEditorHeight, editorWidth / 2, editorHeight);

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
}

void BoardComponent::processorRemoved (const BaseProcessor* proc)
{
    for (auto* editor : processorEditors)
    {
        if (editor->getProcPtr() == proc)
        {
            processorEditors.removeObject (editor);
            refreshBoardSize();
            return;
        }
    }
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
