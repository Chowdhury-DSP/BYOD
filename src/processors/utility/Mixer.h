#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class Mixer : public BaseProcessor
{
public:
    Mixer (UndoManager* um) : BaseProcessor ("Mixer", createParameterLayout(), um, numIns, 1)
    {
        for (int i = 0; i < numIns; ++i)
            gainDBParams[i] = vts.getRawParameterValue ("gain" + String (i));

        uiOptions.backgroundColour = Colours::black;
        uiOptions.powerColour = Colours::yellow;
        uiOptions.info.description = "Mixes together four input channels.";
        uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
    }

    ProcessorType getProcessorType() const override { return Utility; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace ParameterHelpers;
        auto params = createBaseParams();
        for (int i = 0; i < numIns; ++i)
        {
            params.push_back (std::make_unique<VTSParam> ("gain" + String (i),
                                                          "Channel " + String (i + 1),
                                                          String(),
                                                          NormalisableRange<float> { -18.0f, 18.0f },
                                                          0.0f,
                                                          &gainValToString,
                                                          &stringToGainVal));
        }

        return { params.begin(), params.end() };
    }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
        for (auto& gain : gains)
        {
            gain.prepare (spec);
            gain.setRampDurationSeconds (0.01);
        }

        monoBuffer.setSize (1, samplesPerBlock);
        stereoBuffer.setSize (2, samplesPerBlock);
    }

    void processAudio (AudioBuffer<float>& buffer) override
    {
        AudioBuffer<float>* outBuffer = nullptr;
        int numInputsProcessed = 0;
        for (int i = 0; i < numIns; ++i)
        {
            gains[i].setGainDecibels (*gainDBParams[i]);

            if (! inputsConnected.contains (i))
                continue;

            numInputsProcessed++;

            auto& inBuffer = getInputBuffer (i);
            int numChannels = inBuffer.getNumChannels();
            int numSamples = inBuffer.getNumSamples();

            if (outBuffer == nullptr)
            {
                // we haven't chosen an output buffer yet, so choose it here!
                outBuffer = numChannels == 2 ? &stereoBuffer : &monoBuffer;
                outBuffer->setSize (numChannels, numSamples, false, false, true);
                outBuffer->clear();
            }
            else if (outBuffer == &monoBuffer && numChannels == 2)
            {
                // we chose the mono buffer, but need a stereo output, so convert here
                outBuffer = &stereoBuffer;
                outBuffer->setSize (numChannels, numSamples, false, false, true);

                for (int ch = 0; ch < 2; ++ch)
                    outBuffer->copyFrom (ch, 0, monoBuffer, 0, 0, numSamples);
            }

            dsp::AudioBlock<float> block (inBuffer);
            dsp::ProcessContextReplacing<float> ctx (block);
            gains[i].process (ctx);

            for (int ch = 0; ch < outBuffer->getNumChannels(); ++ch)
            {
                int sourceCh = numChannels == 0 ? 0 : ch;
                outBuffer->addFrom (ch, 0, inBuffer, sourceCh, 0, numSamples);
            }
        }

        if (numInputsProcessed == 0)
        {
            buffer.clear();
            outputBuffer = &buffer;
        }
        else if (numInputsProcessed == 1)
        {
            buffer.makeCopyOf (*outBuffer, true);
            outputBuffer = &buffer;
        }
        else
        {
            outputBuffer = outBuffer;
        }
    }

private:
    static constexpr int numIns = 4;
    std::array<std::atomic<float>*, numIns> gainDBParams;
    std::array<dsp::Gain<float>, numIns> gains;

    AudioBuffer<float> monoBuffer;
    AudioBuffer<float> stereoBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mixer)
};
