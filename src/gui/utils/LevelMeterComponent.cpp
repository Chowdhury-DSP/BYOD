#include "LevelMeterComponent.h"

LevelMeterComponent::LevelMeterComponent (const LevelDataType& levelData) : rmsLevels (levelData)
{
    constexpr int timerHz = 24;

    for (int ch = 0; ch < 2; ++ch)
    {
        levelDetector[ch].prepare ({ (double) timerHz, 128, 2 });
        levelDetector[ch].setParameters (80.0f, 300.0f);
    }

    startTimerHz (timerHz);
}

void LevelMeterComponent::paint (Graphics& g)
{
    auto meterBounds = Rectangle { 35, getHeight() - 2 }.withCentre (getLocalBounds().getCentre());

    g.setColour (Colours::black);
    g.fillRoundedRectangle (meterBounds.toFloat(), 4.0f);

    meterBounds.reduce (4, 5);
    const auto height = meterBounds.getHeight();
    //    const auto meterMarkBounds = meterBounds.removeFromLeft (meterBounds.getWidth() / 3);
    const auto leftChBounds = meterBounds.removeFromLeft (meterBounds.getWidth() / 2).translated (-1, 0);
    const auto rightChBounds = meterBounds.translated (1, 0);

    auto getYForDB = [height] (float dB)
    {
        constexpr auto maxDB = 6.0f;
        constexpr auto minDB = -45.0f;
        constexpr auto dBRange = maxDB - minDB;

        auto normLevel = jmin (jmax (dB - minDB, 0.0f) / dBRange, 1.0f);
        return int ((1.0f - normLevel) * (float) height);
    };

    auto gradient = ColourGradient::vertical (Colour (0xFFFF0000), Colour (0xFF00C08B), meterBounds);
    gradient.addColour (0.6f, Colour (0xffF6C80D));
    g.setGradientFill (gradient);

    const auto yPad = meterBounds.getY() / 2;
    g.fillRect (leftChBounds.withTop (getYForDB (dbLevels[0]) + yPad));
    g.fillRect (rightChBounds.withTop (getYForDB (dbLevels[1]) + yPad));

    //    g.setFont (12.0f);
    //    g.setColour (Colours::black);
    //    for (auto dbLevel : { -45.0f, -30.0f, -15.0f, 0.0f })
    //    {
    //        auto y = (float) getYForDB (dbLevel);
    //        auto right = (float) meterMarkBounds.getRight();
    //        g.drawLine (right - 6.0f, y, right, y);
    //
    //        const auto labelBounds = meterMarkBounds.withHeight (20).withCentre (juce::Point { meterMarkBounds.getCentreX(), (int) y });
    //        g.drawFittedText (String ((int) dbLevel), labelBounds, Justification::centred, 1);
    //    }
}

void LevelMeterComponent::timerCallback()
{
    for (int ch = 0; ch < 2; ++ch)
        dbLevels[ch] = Decibels::gainToDecibels (levelDetector[ch].processSample (rmsLevels[ch]));

    repaint();
}
