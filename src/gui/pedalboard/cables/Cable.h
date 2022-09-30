#pragma once

#include "../editors/ProcessorEditor.h"
#include "CubicBezier.h"
#include "processors/BaseProcessor.h"
#include "CableDrawingHelpers.h"
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

private:
    auto createCablePath (juce::Point<float> start, juce::Point<float> end);
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

    juce::Point<float> startPortLocation;
    Colour startColour;
    Colour endColour;
    juce::Point<float> endPortLocation;
    float scaleFactor = 1.0f;
    juce::Range<float> levelRange = {CableConstants::floorDB, 0.0f};
    float levelDB = levelRange.getStart();

    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
