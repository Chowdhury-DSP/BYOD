#pragma once

#include "Cable.h"
#include "InfoComponent.h"
#include "ProcessorEditor.h"
#include "utils/LookAndFeels.h"

class BoardComponent : public Component,
                       private ProcessorChain::Listener,
                       private ProcessorEditor::PortListener
{
public:
    BoardComponent (ProcessorChain& procs);
    ~BoardComponent();

    void paint (Graphics& g) override;
    void resized() override;
    void setScaleFactor (float newScaleFactor);

    void showInfoComp (const BaseProcessor& proc);

    void processorAdded (BaseProcessor* newProc) override;
    void processorRemoved (const BaseProcessor* proc) override;
    void refreshConnections() override;
    void connectionAdded (const ConnectionInfo& info) override;
    void connectionRemoved (const ConnectionInfo& info) override;

    const OwnedArray<ProcessorEditor>& getEditors() { return processorEditors; }
    ProcessorEditor* findEditorForProcessor (const BaseProcessor* proc) const;

    void createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e) override;
    void refreshCable (const MouseEvent& e) override;
    void releaseCable (const MouseEvent& e) override;
    void destroyCable (ProcessorEditor* origin, int portIndex) override;

    static constexpr auto yOffset = 35;

private:
    void showNewProcMenu() const;
    std::pair<ProcessorEditor*, int> getNearestInputPort (const Point<int>& pos) const;
    void setEditorPosition (ProcessorEditor* editor);

    ProcessorChain& procChain;

    TextButton newProcButton;
    OwnedArray<ProcessorEditor> processorEditors;
    InfoComponent infoComp;

    std::unique_ptr<ProcessorEditor> inputEditor;
    std::unique_ptr<ProcessorEditor> outputEditor;

    OwnedArray<Cable> cables;
    std::unique_ptr<MouseEvent> cableMouse;
    bool ignoreConnectionCallbacks = false;

    float scaleFactor = 1.0f;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
