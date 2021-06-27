#pragma once

#include <pch.h>

class BottomBarLNF : public chowdsp::ChowLNF
{
public:
    BottomBarLNF() = default;

    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                              .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour (baseColour);
        g.fillRect (bounds);
    }
};
