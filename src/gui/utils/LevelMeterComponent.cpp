#include "LevelMeterComponent.h"

namespace LevelMeterConstants
{
constexpr auto maxDB = 6.0f;
constexpr auto minDB = -45.0f;
constexpr auto dBRange = maxDB - minDB;
} // namespace LevelMeterConstants

LevelMeterComponent::LevelMeterComponent (const LevelDataType& levelData) : rmsLevels (levelData),
                                                                            dbLevels ({ LevelMeterConstants::minDB, LevelMeterConstants::minDB }),
                                                                            dbLevelsPrev ({ 0.0f, 0.0f })
{
    constexpr int timerHz = 24;

    for (int ch = 0; ch < 2; ++ch)
    {
        levelDetector[ch].prepare ({ (double) timerHz, 128, 2 });
        levelDetector[ch].setParameters (80.0f, 300.0f);
    }

    timerCallback();
    startTimerHz (timerHz);
}

Rectangle<int> LevelMeterComponent::getMeterBounds() const
{
    return Rectangle { 35, getHeight() - 2 }.withCentre (getLocalBounds().getCentre());
}

void LevelMeterComponent::paint (Graphics& g)
{
    auto meterBounds = getMeterBounds();

    g.setColour (Colours::black);
    g.fillRoundedRectangle (meterBounds.toFloat(), 4.0f);

    meterBounds.reduce (4, 5);
    const auto height = meterBounds.getHeight();
    const auto leftChBounds = meterBounds.removeFromLeft (meterBounds.getWidth() / 2).translated (-1, 0);
    const auto rightChBounds = meterBounds.translated (1, 0);

    auto getYForDB = [height] (float dB)
    {
        auto normLevel = jmin (jmax (dB - LevelMeterConstants::minDB, 0.0f) / LevelMeterConstants::dBRange, 1.0f);
        return int ((1.0f - normLevel) * (float) height);
    };

    auto gradient = ColourGradient::vertical (Colour (0xFFFF0000), Colour (0xFF00C08B), meterBounds);
    gradient.addColour (0.6f, Colour (0xffF6C80D));
    g.setGradientFill (gradient);

    const auto yPad = meterBounds.getY() / 2;
    g.fillRect (leftChBounds.withTop (getYForDB (dbLevels[0]) + yPad));
    g.fillRect (rightChBounds.withTop (getYForDB (dbLevels[1]) + yPad));
}

void LevelMeterComponent::timerCallback()
{
    bool needsRepaint = false;
    for (size_t ch = 0; ch < 2; ++ch)
    {
        dbLevels[ch] = Decibels::gainToDecibels (levelDetector[ch].processSample (rmsLevels[ch]));

        if (std::abs (dbLevels[ch] - dbLevelsPrev[ch]) > 0.5f && dbLevels[ch] > LevelMeterConstants::minDB && dbLevelsPrev[ch] > LevelMeterConstants::minDB)
        {
            dbLevelsPrev[ch] = dbLevels[ch];
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint (getMeterBounds());
}
