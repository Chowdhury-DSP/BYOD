#include "LevelMeterComponent.h"

LevelMeterComponent::LevelMeterComponent (const LevelDataType& levelData) : rmsLevels (levelData)
{
    constexpr int timerHz = 24;

    for (int ch = 0; ch < 2; ++ch)
    {
        levelDetector[ch].prepare ({ (double) timerHz, 128, 2 });
        levelDetector[ch].setParameters (100.0f, 500.0f);
    }

    startTimerHz (timerHz);
}

void LevelMeterComponent::paint (Graphics& g)
{
    const auto height = getHeight();
    auto meterBounds = Rectangle { 30, height }.withCentre (getLocalBounds().getCentre());
    meterBounds.reduce (2, 0);
    auto leftChBounds = meterBounds.removeFromLeft (meterBounds.getWidth() / 2).translated (-1, 0);
    auto rightChBounds = meterBounds.translated (1, 0);

    g.setColour (Colours::black);
    g.fillRect (leftChBounds);
    g.fillRect (rightChBounds);

    auto getYForDB = [height] (float dB)
    {
        constexpr auto maxDB = 6.0f;
        constexpr auto minDB = -60.0f;
        constexpr auto dBRange = maxDB - minDB;

        auto normLevel = jmin (jmax (dB - minDB, 0.0f) / dBRange, 1.0f);
        return int ((1.0f - normLevel) * (float) height);
    };

    auto gradient = ColourGradient::vertical (Colour (0xFFFF0000), Colour (0xFF00C08B), meterBounds);
    gradient.addColour (0.6f, Colour (0xffF6C80D));
    g.setGradientFill (gradient);

    leftChBounds = leftChBounds.withTop (getYForDB (dbLevels[0]));
    g.fillRect (leftChBounds);

    rightChBounds = rightChBounds.withTop (getYForDB (dbLevels[1]));
    g.fillRect (rightChBounds);
}

void LevelMeterComponent::timerCallback()
{
    for (int ch = 0; ch < 2; ++ch)
        dbLevels[ch] = Decibels::gainToDecibels (levelDetector[ch].processSample (rmsLevels[ch]));

    repaint();
}
