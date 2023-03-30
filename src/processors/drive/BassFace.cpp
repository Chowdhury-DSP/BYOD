#include "BassFace.h"
#include "../ParameterHelpers.h"

BassFace::BassFace (UndoManager* um) : BaseProcessor ("Bass Face", createParameterLayout(), um)
{
    gainSmoothed.setParameterHandle (dynamic_cast<chowdsp::FloatParameter*> (vts.getParameter ("gain")));

    uiOptions.backgroundColour = Colours::darkred.brighter (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "Emulation of a HEAVY bass distortion signal chain.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout BassFace::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "gain", "Gain", 0.5f);

    return { params.begin(), params.end() };
}

void BassFace::prepare (double sampleRate, int samplesPerBlock)
{
    if ((int) sampleRate % 44100 == 0)
    {
        for (auto& m : model)
            m.initialise (BinaryData::bass_face_model_88_2k_json, BinaryData::bass_face_model_88_2k_jsonSize, 88200.0);
    }
    else
    {
        for (auto& m : model)
            m.initialise (BinaryData::bass_face_model_96k_json, BinaryData::bass_face_model_96k_jsonSize, 96000.0);
    }

    const size_t oversamplingOrder = sampleRate <= 48000.0 ? 1 : 0;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, oversamplingOrder, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampling->initProcessing (samplesPerBlock);
    const auto osSampleRate = sampleRate * (double) oversampling->getOversamplingFactor();
    const auto osSamplesPerBlock = samplesPerBlock * (int) oversampling->getOversamplingFactor();

    for (auto& m : model)
        m.prepare (osSampleRate, osSamplesPerBlock);

    gainSmoothed.prepare (osSampleRate, (int) std::ceil ((float) osSamplesPerBlock));
    gainSmoothed.setRampLength (0.05);

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (15.0f, chowdsp::CoefficientCalculators::butterworthQ<float>, (float) sampleRate);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 5000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void BassFace::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();

    {
        auto&& block = dsp::AudioBlock<float> { buffer };
        auto&& osBlock = oversampling->processSamplesUp (block);

        const auto osNumSamples = (int) osBlock.getNumSamples();
        gainSmoothed.process (osNumSamples);
        const auto* gainData = gainSmoothed.getSmoothedBuffer();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto&& channelBlock = osBlock.getSingleChannelBlock ((size_t) ch);
            model[ch].process (channelBlock, { gainData, (size_t) osNumSamples });
        }

        oversampling->processSamplesDown (block);
    }

    dcBlocker.processBlock (buffer);
}
