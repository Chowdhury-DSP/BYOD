#pragma once

#include "processors/BaseProcessor.h"
#include "../editors/ProcessorEditor.h"
#include "../editors/ProcessorEditor.h"
#include <pch.h> //do i need this?
#include "CubicBezier.h"


class CableViewConnectionHelper;
class CableViewPortLocationHelper;
class Cable: public Component
{
    public:
    
    Cable (BaseProcessor* start, int startPort);
    Cable (BaseProcessor* start, int startPort, BaseProcessor* end, int endPort);
    ~Cable() override;
    
    void paint (Graphics& g) override;
        
    auto createCablePath (Point<float> start, Point<float> end, float scaleFactor);
    
    float getCableThickness (float levelDB);
    
    void drawCableShadow (Graphics& g, const Path& cablePath, float thickness);
    
    void drawCable (Graphics& g, Point<float> start, Colour startColour, Point<float> end, Colour endColour, float scaleFactor, float levelDB);
    
    void drawCableEndCircle (Graphics& g, Point<float> centre, Colour colour, float scaleFactor);
    
    bool hitTest(int x, int y) override;

    BaseProcessor* startProc = nullptr;//getters and setterss
    int startIdx;

    BaseProcessor* endProc = nullptr;
    int endIdx = 0;

private:
    
    Path cablePath;//should this be in initializer list and const
    int nump;
    CubicBezier bezier;
    float cablethickness;
    chowdsp::PopupMenuHelper popupMenu;
    
    
    const Colour cableColour; // currently only used for "glow"
    static constexpr float cableThickness = 5.0f;
    static constexpr float portCircleThickness = 1.5f;

    static constexpr int getPortDistanceLimit (float scaleFactor) { return int (20.0f * scaleFactor); }
    static constexpr auto portOffset = 50.0f;

    static constexpr float floorDB = -60.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
