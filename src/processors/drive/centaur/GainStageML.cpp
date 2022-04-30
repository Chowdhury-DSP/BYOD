#include "GainStageML.h"

GainStageML::GainStageML (AudioProcessorValueTreeState& vts)
{
    loadModel (gainStageML[0], BinaryData::centaur_0_json, BinaryData::centaur_0_jsonSize);
    loadModel (gainStageML[1], BinaryData::centaur_25_json, BinaryData::centaur_25_jsonSize);
    loadModel (gainStageML[2], BinaryData::centaur_50_json, BinaryData::centaur_50_jsonSize);
    loadModel (gainStageML[3], BinaryData::centaur_75_json, BinaryData::centaur_75_jsonSize);
    loadModel (gainStageML[4], BinaryData::centaur_100_json, BinaryData::centaur_100_jsonSize);

    gainParam = vts.getRawParameterValue ("gain");
}

void GainStageML::loadModel (ModelPair& model, const char* data, int size)
{
    for (auto& channelModel : model)
        channelModel.initialise (data, size, 44100.0);
}

void GainStageML::reset (double sampleRate, int samplesPerBlock)
{
    fadeBuffer.setSize (2, samplesPerBlock);

    for (int i = 0; i < numModels; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            gainStageML[i][ch].prepare (sampleRate, samplesPerBlock);

            // pre-buffer to avoid "click" on initialisation
            for (int k = 0; k < int (0.1 * sampleRate); k += samplesPerBlock)
            {
                fadeBuffer.clear();
                auto&& preBufferBlock = dsp::AudioBlock<float> { fadeBuffer }.getSingleChannelBlock (0);
                gainStageML[i][ch].process (preBufferBlock);
            }
        }
    }

    fadeBuffer.clear();
    lastModelIdx = getModelIdx();
}

void GainStageML::processModel (AudioBuffer<float>& buffer, ModelPair& model)
{
    auto&& block = dsp::AudioBlock<float> { buffer };
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto&& channelBlock = block.getSingleChannelBlock ((size_t) ch);
        model[ch].process (channelBlock);
    }
}

void GainStageML::processBlock (AudioBuffer<float>& buffer)
{
    const auto modelIdx = getModelIdx();

    //    processModel (buffer, gainStageML[modelIdx]);

    if (modelIdx == lastModelIdx)
    {
        processModel (buffer, gainStageML[modelIdx]);
    }
    else // need to fade between models
    {
        fadeBuffer.makeCopyOf (buffer, true);
        processModel (buffer, gainStageML[lastModelIdx]); // previous model
        processModel (fadeBuffer, gainStageML[modelIdx]); // next model

        buffer.applyGainRamp (0, buffer.getNumSamples(), 1.0f, 0.0f);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFromWithRamp (ch, 0, fadeBuffer.getReadPointer (ch), buffer.getNumSamples(), 0.0f, 1.0f);
    }

    lastModelIdx = modelIdx;
}
