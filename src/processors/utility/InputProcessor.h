#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class InputProcessor : public BaseProcessor
{
public:
    explicit InputProcessor (UndoManager* um = nullptr) : BaseProcessor ("Input", createParameterLayout(), um, 0, 1)
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
                                                                  StringArray { "Mono", "Stereo", "Left", "Right" },
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
        const auto useStereo = monoModeParam->load() == 1.0f;
        const auto useLeft = monoModeParam->load() == 2.0f;
        const auto useRight = monoModeParam->load() == 3.0f;

        // simple case: stereo input!
        if (useStereo)
        {
            outputBuffers.getReference (0) = &buffer;
            return;
        }

        // Mono output, but which mono?
        monoBuffer.setSize (1, numSamples, false, false, true);
        if (useMono)
        {
            monoBuffer.clear();
            for (int ch = 0; ch < numChannels; ++ch)
                monoBuffer.addFrom (0, 0, buffer.getReadPointer (ch), numSamples);

            monoBuffer.applyGain (1.0f / (float) numChannels);
        }
        else if (useLeft)
        {
            monoBuffer.copyFrom (0, 0, buffer.getReadPointer (0), numSamples);
        }
        else if (useRight)
        {
            monoBuffer.copyFrom (0, 0, buffer.getReadPointer (1 % numChannels), numSamples);
        }

        outputBuffers.getReference (0) = &monoBuffer;
    }

private:
    std::atomic<float>* monoModeParam = nullptr;

    AudioBuffer<float> monoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputProcessor)
};
