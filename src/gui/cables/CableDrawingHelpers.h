#pragma once

#include <pch.h>

namespace CableConstants
{
const Colour cableColour (0xFFD0592C); // currently only used for "glow"
constexpr float cableThickness = 5.0f;
constexpr float portCircleThickness = 1.5f;

constexpr int portDistanceLimit = 25;
constexpr auto portOffset = 50.0f;

constexpr float floorDB = -60.0f;
} // namespace CableConstants

namespace CableDrawingHelpers
{
void drawCable (Graphics& g, Point<float> start, Colour startColour, Point<float> end, Colour endColour, float scaleFactor, float levelDB = CableConstants::floorDB);

void drawCablePortGlow (Graphics& g, Point<int> location);

} // namespace CableDrawingHelpers
