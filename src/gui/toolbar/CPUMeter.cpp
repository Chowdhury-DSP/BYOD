#include "CPUMeter.h"

class CPUMeterLNF : public LookAndFeel_V4
{
public:
    CPUMeterLNF() = default;
    ~CPUMeterLNF() override = default;

    void drawProgressBar (Graphics& g, ProgressBar& progressBar, int width, int height, double progress, const String& textToShow) override
    {
        auto background = progressBar.findColour (ProgressBar::backgroundColourId);
        auto foreground = progressBar.findColour (ProgressBar::foregroundColourId);

        auto barBounds = progressBar.getLocalBounds().toFloat();
        const auto cornerSize = (float) progressBar.getHeight() * 0.1f;

        g.setColour (background);
        g.fillRoundedRectangle (barBounds, cornerSize);

        {
            Path p;
            p.addRoundedRectangle (barBounds, cornerSize);
            g.reduceClipRegion (p);

            barBounds.setWidth (barBounds.getWidth() * (float) progress);
            g.setColour (foreground);
            g.fillRoundedRectangle (barBounds, cornerSize);
        }

        if (textToShow.isNotEmpty())
        {
            g.setColour (Colours::white);
            g.setFont ((float) height * 0.6f);

            g.drawText (textToShow, 0, 0, width, height, Justification::centred, false);
        }
    }
};

CPUMeter::CPUMeter (AudioProcessLoadMeasurer& lm) : loadMeasurer (lm)
{
    setLookAndFeel (lnfAllocator->addLookAndFeel<CPUMeterLNF>());

    addAndMakeVisible (progress);

    constexpr double sampleRate = 20.0;
    startTimerHz ((int) sampleRate);

    smoother.prepare ({ sampleRate, 128, 1 });
    smoother.setParameters (500.0f, 1000.0f);
}

void CPUMeter::colourChanged()
{
    progress.setColour (ProgressBar::backgroundColourId, findColour (ProgressBar::backgroundColourId));
    progress.setColour (ProgressBar::foregroundColourId, findColour (ProgressBar::foregroundColourId));
}

void CPUMeter::timerCallback()
{
    loadProportion = smoother.processSample (loadMeasurer.getLoadAsProportion());
}

void CPUMeter::resized()
{
    progress.setBounds (getLocalBounds());
}
