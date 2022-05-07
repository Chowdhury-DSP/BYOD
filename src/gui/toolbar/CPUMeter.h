#pragma once

#include <pch.h>

class CPUMeter : public Component,
                 private Timer
{
public:
    explicit CPUMeter (AudioProcessLoadMeasurer& loadMeasurer);

    void colourChanged() override;
    void resized() override;

private:
    void timerCallback() override;

    double loadProportion = 0.0;
    ProgressBar progress { loadProportion };
    chowdsp::LevelDetector<double> smoother;

    AudioProcessLoadMeasurer& loadMeasurer;

    chowdsp::SharedLNFAllocator lnfAllocator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CPUMeter)
};


