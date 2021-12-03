#pragma once

#include "CableView.h"
#include "InfoComponent.h"
#include "ProcessorEditor.h"
#include "utils/LookAndFeels.h"

class BoardComponent final : public Component,
                             private ProcessorChain::Listener
{
public:
    explicit BoardComponent (ProcessorChain& procs);
    ~BoardComponent() override;

    void resized() override;
    void setScaleFactor (float newScaleFactor);

    void showInfoComp (const BaseProcessor& proc);

    void processorAdded (BaseProcessor* newProc) override;
    void processorRemoved (const BaseProcessor* proc) override;

    const OwnedArray<ProcessorEditor>& getEditors() { return processorEditors; }
    ProcessorEditor* findEditorForProcessor (const BaseProcessor* proc) const;

    static constexpr auto yOffset = 35;

private:
    void showNewProcMenu() const;
    void setEditorPosition (ProcessorEditor* editor);

    ProcessorChain& procChain;

    TextButton newProcButton;
    OwnedArray<ProcessorEditor> processorEditors;
    InfoComponent infoComp;

    std::unique_ptr<ProcessorEditor> inputEditor;
    std::unique_ptr<ProcessorEditor> outputEditor;

    friend class CableView;
    CableView cableView;

    float scaleFactor = 1.0f;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
