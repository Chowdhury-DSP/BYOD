#pragma once

#include <pch.h>

class UIState : private ComponentListener
{
public:
    explicit UIState (AudioProcessorValueTreeState& vts);

    void attachToComponent (Component& comp);
    std::pair<int, int> getLastEditorSize() const;

private:
    void componentBeingDeleted (Component& component) override;
    void componentMovedOrResized (Component& component, bool wasMoved, bool wasResized) override;
    void setLastEditorSize (int width, int height);

    AudioProcessorValueTreeState& vts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UIState)
};
