#pragma once

#include "../editors/ProcessorEditor.h"
#include "Cable.h"

class Cable;
class BoardComponent;
class CableViewConnectionHelper;
class CableViewPortLocationHelper;
class CableView : public Component,
                  private Timer
{
public:
    explicit CableView (BoardComponent* comp);//keeping this board component const would be ideal
    ~CableView() override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

    auto* getConnectionHelper() { return connectionHelper.get(); }
    auto* getPortLocationHelper() { return portLocationHelper.get(); }
    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingAddedFromCableClick (BaseProcessor* newProc, Cable* c);
    void processorBeingRemoved (const BaseProcessor* proc);

    struct EditorPort
    {
        ProcessorEditor* editor = nullptr;
        int portIndex = 0;
        bool isInput = false;
    };
    
    std::unique_ptr<MouseEvent> cableMouse = nullptr;
    bool mouseClicked = false; // Used within the Cable hitTest() method, because each cables bounds are the size of
                               // CableView, each Cable hitTest() method is always activated when a mouse is on screen
                               // this provides a bool to check so Cable hitTest() is not always checking.

private:
    void timerCallback() override;
    BoardComponent* board = nullptr; //should be const
    OwnedArray<Cable> cables;

    float scaleFactor = 1.0f;
    bool isDraggingCable = false;

    friend class CableViewConnectionHelper;
    std::unique_ptr<CableViewConnectionHelper> connectionHelper;

    friend class CableViewPortLocationHelper;
    std::unique_ptr<CableViewPortLocationHelper> portLocationHelper;

    EditorPort nearestPort {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableView)
};
