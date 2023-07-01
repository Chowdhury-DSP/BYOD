#pragma once

#include "../BaseProcessor.h"

class StereoSplitter : public BaseProcessor
{
public:
    explicit StereoSplitter (UndoManager* um);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    String getTooltipForPort (int portIndex, bool isInput) override;

private:
    enum OutputPort
    {
        LeftChannel,
        RightChannel,
    };
    static constexpr int numOuts = magic_enum::enum_count<OutputPort>();

    std::atomic<float>* modeParam = nullptr;

    AudioBuffer<float> buffers[numOuts];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoSplitter)
};
