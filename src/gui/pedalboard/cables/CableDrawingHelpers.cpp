#include "CableDrawingHelpers.h"

namespace CableDrawingHelpers
{
using namespace CableConstants;

#if JUCE_IOS
constexpr float glowSizeFactor = 4.5f;
#else
constexpr float glowSizeFactor = 2.5f;
#endif

juce::Rectangle<float> getPortGlowBounds (juce::Point<int> location, float scaleFactor)
{
    const auto glowDim = (float) getPortDistanceLimit (scaleFactor) * glowSizeFactor;
    auto glowBounds = (juce::Rectangle (glowDim, glowDim)).withCentre (location.toFloat());

    return glowBounds;
}

void drawCablePortGlow (Graphics& g, juce::Point<int> location, float scaleFactor)
{
    Graphics::ScopedSaveState graphicsState (g);
    g.setColour (cableColour.darker (0.1f));
    g.setOpacity (0.65f);

    g.fillEllipse (getPortGlowBounds (location, scaleFactor));
}
} // namespace CableDrawingHelpers
