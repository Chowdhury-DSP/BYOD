#include "BoardComponent.h"

namespace
{
    constexpr int editorWidth = 300;
    constexpr int editorHeight = 200;
    constexpr int editorPad = 10;
    constexpr int newButtonWidth = 40;
    constexpr int newButtonPad = 10;
}

BoardComponent::BoardComponent (ProcessorChain& procs) : procChain (procs)
{
    newProcButton.setButtonText ("+");
    addAndMakeVisible (newProcButton);
    newProcButton.onClick = [=] { showNewProcMenu(); };

    for (auto* p : procs.getProcessors())
        processorAdded (p);

    procChain.addListener (this);
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
    g.fillAll (Colours::slategrey.darker());
}

void BoardComponent::resized()
{
    auto bounds = getLocalBounds();

    for (auto* editor : processorEditors)
    {
        auto b = bounds.removeFromLeft (editorWidth + 2 * editorPad);
        auto height = jmin (editorHeight, getHeight() - 2 * editorPad);
        editor->setBounds (Rectangle<int> (editorWidth, height).withCentre (b.getCentre()));
    }

    // draw newProcButton
    {
        const auto centre = bounds.getCentre();
        newProcButton.setBounds (Rectangle<int> (newButtonWidth, newButtonWidth).withCentre (centre));
    }

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
    auto* newEditor = processorEditors.add (std::make_unique<ProcessorEditor> (*newProc, procChain));
    addAndMakeVisible (newEditor);

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

bool BoardComponent::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    return processorEditors.contains (dynamic_cast<ProcessorEditor*> (dragSourceDetails.sourceComponent.get()));
}

void BoardComponent::itemDropped (const SourceDetails& dragSourceDetails)
{
    const auto dragX = dragSourceDetails.localPosition.x;
    if (dragX < 0 || dragX > getWidth())
        return;

    int lastX = 0;
    for (const auto* editor : processorEditors)
    {
        auto newX = editor->getRight();
        if (dragX >= lastX && dragX < newX)
        {
            auto* draggedEditor = dynamic_cast<ProcessorEditor*> (dragSourceDetails.sourceComponent.get());
            procChain.moveProcessor (draggedEditor->getProcPtr(), editor->getProcPtr());
            return;
        }

        lastX = newX;
    }

    if (lastX > 0)
    {
        auto* draggedEditor = dynamic_cast<ProcessorEditor*> (dragSourceDetails.sourceComponent.get());
        procChain.moveProcessor (draggedEditor->getProcPtr(), nullptr);
    }
}

void BoardComponent::showNewProcMenu() const
{
    auto& procStore = procChain.getProcStore();

    int menuID = 0;
    PopupMenu menu;
    for (auto type : { Drive, Tone, Other })
    {
        PopupMenu subMenu;
        procStore.createProcList (subMenu, menuID, type);

        auto typeName = std::string (magic_enum::enum_name (type));
        menu.addSubMenu (String (typeName), subMenu);
    }

    menu.showMenuAsync(PopupMenu::Options().withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards));
}
