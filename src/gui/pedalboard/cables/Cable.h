#pragma once

#include "../editors/ProcessorEditor.h"
#include "CableView.h"
#include "CubicBezier.h"
#include "processors/BaseProcessor.h"
#include <pch.h>

class BoardComponent;
class CableView;
class CableViewConnectionHelper;
class CableViewPortLocationHelper;
class Cable : public Component
{
public:
    Cable (const BoardComponent* comp, const CableView& cv, const ConnectionInfo connection);
    ~Cable() override;

    auto createCablePath (juce::Point<float> start, juce::Point<float> end, float scaleFactor);
    float getCableThickness (float levelDB);
    void drawCableShadow (Graphics& g, float thickness);
    void drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour);
    void drawCable (Graphics& g, juce::Point<float> start, Colour startColour, juce::Point<float> end, Colour endColour);
    void paint (Graphics& g) override;
    bool hitTest (int x, int y) override;

    BaseProcessor* startProc = nullptr;
    int startIdx = 0;

    BaseProcessor* endProc = nullptr;
    int endIdx = 0;

private:
    const CableView& cableView;
    const BoardComponent* board = nullptr;

    Path cablePath;
    int numPointsInPath;
    CubicBezier bezier;
    float cablethickness;

    juce::Point<float> startPortLocation;
    Colour startColour;
    Colour endColour;
    juce::Point<float> endPortLocation;
    float scaleFactor;
    float levelDB;

    const Colour cableColour; // currently only used for "glow"
    static constexpr float cableThickness = 5.0f;
    static constexpr float portCircleThickness = 1.5f;

    static constexpr int getPortDistanceLimit (float scaleFactor) { return int (20.0f * scaleFactor); }
    static constexpr auto portOffset = 50.0f;

    static constexpr float floorDB = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
