#pragma once

#include "../editors/ProcessorEditor.h"
#include "Cable.h"

class BoardComponent;
class CableViewConnectionHelper;
class CableViewPortLocationHelper;
class CableView : public Component,
                  private Timer
{
public:
    explicit CableView (BoardComponent* comp);
    ~CableView() override;

    void paint (Graphics& g) override;
    void resized() override;

    void mouseMove (const MouseEvent& e) override;
    void mouseExit (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

    auto* getConnectionHelper() { return connectionHelper.get(); }
    auto* getPortLocationHelper() { return portLocationHelper.get(); }
    void processorBeingAdded (BaseProcessor* newProc);
    void processorBeingRemoved (const BaseProcessor* proc);

    bool cableBeingDragged() const;
    juce::Point<float> getCableMousePosition() const;

    struct EditorPort
    {
        ProcessorEditor* editor = nullptr;
        int portIndex = 0;
        bool isInput = false;
    };

private:
    void timerCallback() override;

    BoardComponent* board = nullptr;
    OwnedArray<Cable> cables;

    float scaleFactor = 1.0f;
    bool isDraggingCable = false;
    std::optional<juce::Point<int>> mousePosition;

    friend class CableViewConnectionHelper;
    std::unique_ptr<CableViewConnectionHelper> connectionHelper;

    friend class CableViewPortLocationHelper;
    std::unique_ptr<CableViewPortLocationHelper> portLocationHelper;

    EditorPort nearestPort {};
    juce::Point<int> portToPaint;

    bool mouseOverClickablePort();
    bool mouseDraggingOverOutputPort();

    CriticalSection cableMutex;

    bool portGlow = false;

    struct PathGeneratorTask : juce::TimeSliceClient
    {
    public:
        explicit PathGeneratorTask (CableView& cv);
        ~PathGeneratorTask() override;

        int useTimeSlice() override;

    private:
        struct TimeSliceThread : juce::TimeSliceThread
        {
            TimeSliceThread() : juce::TimeSliceThread ("Cable Drawing Background Thread") {}
        };

        juce::SharedResourcePointer<TimeSliceThread> sharedTimeSliceThread;

        CableView& cableView;
    } pathTask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableView)
};
