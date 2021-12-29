#pragma once

#include <pch.h>

template <typename Plugin>
class CPUMeterItem : public foleys::GuiItem,
                     private Timer
{
public:
    FOLEYS_DECLARE_GUI_FACTORY (CPUMeterItem)

    CPUMeterItem (foleys::MagicGUIBuilder& builder, const ValueTree& node) : foleys::GuiItem (builder, node),
                                                                             loadMeasurer (dynamic_cast<Plugin*> (builder.getMagicState().getProcessor())->getLoadMeasurer())
    {
        setColourTranslation ({
            { "background", ProgressBar::backgroundColourId },
            { "foreground", ProgressBar::foregroundColourId },
        });

        addAndMakeVisible (progress);

        constexpr double sampleRate = 20.0;
        startTimerHz ((int) sampleRate);

        smoother.prepare ({ sampleRate, 128, 1 });
        smoother.setParameters (500.0f, 1000.0f);
    }

    void update() override {}

    void timerCallback() override
    {
        loadProportion = smoother.processSample (loadMeasurer.getLoadAsProportion());
    }

    Component* getWrappedComponent() override
    {
        return &progress;
    }

private:
    double loadProportion = 0.0;
    ProgressBar progress { loadProportion };
    chowdsp::LevelDetector<double> smoother;

    AudioProcessLoadMeasurer& loadMeasurer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CPUMeterItem)
};

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
