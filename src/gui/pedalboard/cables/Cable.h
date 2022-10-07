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
    void resized() override;
    bool hitTest (int x, int y) override;

    ConnectionInfo connectionInfo;

    static constexpr std::string_view componentName = "BYOD_Cable";

    void checkNeedsRepaint (bool force = false);

    void updateStartPoint (bool repaintIfMoved = true);
    void updateEndPoint (bool repaintIfMoved = true);

private:
    float getCableThickness() const;
    void drawCableShadow (Graphics& g, float thickness);
    void drawCableEndCircle (Graphics& g, juce::Point<float> centre, Colour colour) const;
    void drawCable (Graphics& g, juce::Point<float> start, juce::Point<float> end);
    CableView& cableView;
    const BoardComponent* board = nullptr;

    chowdsp::PopupMenuHelper popupMenu;

    Path cablePath {};
    int numPointsInPath = 0;
    CubicBezier bezier;
    float cableThickness = 0.0f;

    using AtomicPoint = std::atomic<juce::Point<float>>;
    static_assert (AtomicPoint::is_always_lock_free, "Atomic point needs to be lock free!");
    AtomicPoint startPoint {};
    AtomicPoint endPoint {};
    std::atomic<float> scaleFactor = 1.0f;

    Colour startColour;
    Colour endColour;
    juce::Range<float> levelRange = { CableConstants::floorDB, 0.0f };
    float levelDB = levelRange.getStart();

    Path createCablePath (juce::Point<float> start, juce::Point<float> end, float scaleFactor);
    CriticalSection pathCrit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cable)
};
