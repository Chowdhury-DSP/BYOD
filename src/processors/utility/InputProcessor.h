#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class InputProcessor : public BaseProcessor
{
public:
    InputProcessor (UndoManager* um = nullptr) : BaseProcessor ("Input", createParameterLayout(), um, 0, 1)
    {
        monoModeParam = vts.getRawParameterValue ("mono_stereo");

        uiOptions.backgroundColour = Colours::orange;
    }

    ~InputProcessor() override = default;

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();
        params.push_back (std::make_unique<AudioParameterChoice> ("mono_stereo",
                                                                  "Mono/Stereo",
                                                                  StringArray { "Mono", "Stereo" },
                                                                  0));

        return { params.begin(), params.end() };
    }

    void prepare (double /*sampleRate*/, int samplesPerBlock) override
    {
        monoBuffer.setSize (1, samplesPerBlock);
    }

    void processAudio (AudioBuffer<float>& buffer) override
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();
        const auto useMono = monoModeParam->load() == 0.0f;

        if (useMono)
        {
            monoBuffer.setSize (1, numSamples, false, false, true);
            monoBuffer.clear();

            for (int ch = 0; ch < numChannels; ++ch)
                monoBuffer.addFrom (0, 0, buffer.getReadPointer (ch), numSamples);

            monoBuffer.applyGain (1.0f / (float) numChannels);

            outputBuffers.getReference (0) = &monoBuffer;
        }
        else
        {
            outputBuffers.getReference (0) = &buffer;
        }
    }

private:
    std::atomic<float>* monoModeParam = nullptr;

    AudioBuffer<float> monoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputProcessor)
};
