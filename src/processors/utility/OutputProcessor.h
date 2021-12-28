#pragma once

#include "../BaseProcessor.h"

class OutputProcessor : public BaseProcessor
{
public:
    explicit OutputProcessor (UndoManager* um = nullptr) : BaseProcessor ("Output", createParameterLayout(), um, 1, 0)
    {
        uiOptions.backgroundColour = Colours::lightskyblue;
    }

    ~OutputProcessor() override = default;

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();
        return { params.begin(), params.end() };
    }

    void prepare (double /*sampleRate*/, int samplesPerBlock) override
    {
        stereoBuffer.setSize (2, samplesPerBlock);
    }

    void processAudio (AudioBuffer<float>& buffer) override
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        if (numChannels == 1)
        {
            auto processedData = buffer.getReadPointer (0);
            for (int ch = 0; ch < stereoBuffer.getNumChannels(); ++ch)
                FloatVectorOperations::copy (stereoBuffer.getWritePointer (ch),
                                             processedData,
                                             numSamples);

            outputBuffers.getReference (0) = &stereoBuffer;
        }
        else
        {
            outputBuffers.getReference (0) = &buffer;
        }
    }

private:
    AudioBuffer<float> stereoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputProcessor)
};
