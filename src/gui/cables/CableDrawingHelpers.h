#pragma once

#include <pch.h>

namespace CableConstants
{
const Colour cableColour (0xFFD0592C);
constexpr float cableThickness = 5.0f;

constexpr int portDistanceLimit = 25;
constexpr auto portOffset = 50.0f;
} // namespace CableConstants

namespace CableDrawingHelpers
{

void drawCable (Graphics& g, Point<float> start, Point<float> end, float scaleFactor);

void drawCablePortGlow (Graphics& g, Point<int> location);

} // namespace CableDrawingHelpers
