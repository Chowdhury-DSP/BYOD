#pragma once

#include "../editors/ProcessorEditor.h"
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

    void mouseDown (const MouseEvent& e) override;

    void paint (Graphics& g) override;
    bool hitTest (int x, int y) override; 
    
    BaseProcessor* startProc = nullptr;
    int startIdx = 0;

    BaseProcessor* endProc = nullptr;
    int endIdx = 0;

private:
    auto createCablePath (juce::Point<float> start, juce::Point<float> end, float scaleFactor);
    float getCableThickness();
    void drawCableShadow (Graphics& g, float thickness);
    void drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour);
    void drawCable (Graphics& g, juce::Point<float> start, juce::Point<float> end);
    CableView& cableView;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
