#pragma once

#include "../editors/ProcessorEditor.h"
#include "CableDrawingHelpers.h"
#include "CubicBezier.h"
#include "processors/BaseProcessor.h"
#include <pch.h>

class BoardComponent;
class CableView;
class Cable : public Component
{
public:
    Cable (const BoardComponent* comp, CableView& cv, const ConnectionInfo connection);
    ~Cable() override;

    void paint (Graphics& g) override;
    bool hitTest (int x, int y) override;

    ConnectionInfo connectionInfo;

    static constexpr std::string_view componentName = "BYOD_Cable";

    void checkNeedsRepaint();
    
    void updateStartPoint();
    void updateEndPoint();

private:
    float getCableThickness();
    void drawCableShadow (Graphics& g, float thickness);
    void drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour) const;
    void drawCable (Graphics& g, juce::Point<float> start, juce::Point<float> end);
    CableView& cableView;
    const BoardComponent* board = nullptr;

    chowdsp::PopupMenuHelper popupMenu;

    Path cablePath {};
    int numPointsInPath = 0;
    CubicBezier bezier;
    float cablethickness = 0.0f;

    std::atomic<juce::Point<float>> startLocation;
    std::atomic<juce::Point<float>> endLocation;
    std::atomic<float> scaleFactor = 1.0f;
    
    juce::Rectangle<int> cableBounds;
    Colour startColour;
    Colour endColour;
    juce::Range<float> levelRange = { CableConstants::floorDB, 0.0f };
    float levelDB = levelRange.getStart();

    Path createCablePath (juce::Point<float> start, juce::Point<float> end, float scaleFactor);
    CriticalSection pathCrit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
