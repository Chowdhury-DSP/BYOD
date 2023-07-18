#pragma once

#include "../BaseProcessor.h"
#include "../../modules/chowdsp_utils/modules/gui/chowdsp_visualizers/CompressorPlots/chowdsp_LevelDetectorVisualizer.h"

using namespace chowdsp::compressor;

class LevelDetective : public BaseProcessor
{
public:
    explicit LevelDetective (UndoManager* um);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:

    enum InputPort
    {
        AudioInput
    };

    enum OutputPort
    {
        LevelOutput
    };

//    chowdsp::FloatParameter* attackMsParam = nullptr;
//    chowdsp::FloatParameter* releaseMsParam = nullptr;

    AudioBuffer<float> levelOutBuffer;
    chowdsp::LevelDetector<float> level;
    LevelDetectorVisualizer levelVisualizer {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelDetective)
};
