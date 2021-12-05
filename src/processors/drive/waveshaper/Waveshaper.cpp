#include "Waveshaper.h"
#include "../../ParameterHelpers.h"

using namespace SurgeWaveshapers;

Waveshaper::Waveshaper (UndoManager* um) : BaseProcessor ("Waveshaper", createParameterLayout(), um)
{
    driveParam = vts.getRawParameterValue ("drive");
    shapeParam = vts.getRawParameterValue ("shape");

    // borrowed from: https://github.com/surge-synthesizer/surge/blob/main/src/surge-fx/SurgeLookAndFeel.h
    const Colour surgeOrange = Colour (255, 144, 0);
    const Colour surgeBlue = Colour (18, 52, 99);

    uiOptions.backgroundColour = surgeBlue;
    uiOptions.powerColour = surgeOrange;
    uiOptions.info.description = "Waveshaping effects borrowed from the venerable Surge Synthesizer.";
    uiOptions.info.authors = StringArray { "Surge Synthesizer Team" };
    uiOptions.info.infoLink = "https://surge-synthesizer.github.io";
}

AudioProcessorValueTreeState::ParameterLayout Waveshaper::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createGainDBParameter (params, "drive", "Drive", -6.0f, 36.0f, 0.0f);

    params.push_back (std::make_unique<AudioParameterChoice> ("shape", "Shape", wst_names, wst_ojd));

    return { params.begin(), params.end() };
}

void Waveshaper::prepare (double sampleRate, int samplesPerBlock)
{
    driveSmooth.reset (sampleRate, 0.05);
    driveSmooth.setCurrentAndTargetValue (Decibels::decibelsToGain (driveParam->load()));

    inverseGain.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    inverseGain.setRampDurationSeconds (0.05);
}

void Waveshaper::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    auto driveGain = Decibels::decibelsToGain (driveParam->load());
    driveSmooth.setTargetValue (driveGain);
    //    inverseGain.setGainLinear (1.0f / std::sqrt (driveGain));
    inverseGain.setGainLinear (1.0f / driveGain);

    if ((int) *shapeParam != lastShape)
    {
        lastShape = (int) *shapeParam;

        float R[n_waveshaper_registers];
        initializeWaveshaperRegister (lastShape, R);
        for (int i = 0; i < n_waveshaper_registers; ++i)
            wss.R[i] = Vec4 (R[i]);
        wss.init = dsp::SIMDRegister<uint32_t> (0);
    }

    auto wsptr = GetQFPtrWaveshaper (lastShape);

    if (wsptr)
    {
        float din alignas (16)[4] = { 0, 0, 0, 0 };
        if (numChannels == 1)
        {
            auto* data = buffer.getWritePointer (0);

            for (int i = 0; i < numSamples; ++i)
            {
                din[0] = data[i];
                auto dat = Vec4::fromRawArray (din);
                auto drv = Vec4 (driveSmooth.getNextValue());

                dat = wsptr (&wss, dat, drv);

                float res alignas (16)[4];
                dat.copyToRawArray (res);

                data[i] = res[0];
            }
        }
        else if (numChannels == 2)
        {
            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);

            for (int i = 0; i < numSamples; ++i)
            {
                din[0] = left[i];
                din[1] = right[i];
                auto dat = Vec4::fromRawArray (din);
                auto drv = Vec4 (driveSmooth.getNextValue());

                dat = wsptr (&wss, dat, drv);

                float res alignas (16)[4];
                dat.copyToRawArray (res);

                left[i] = res[0];
                right[i] = res[1];
            }
        }
    }

    auto block = dsp::AudioBlock<float> { buffer };
    auto context = dsp::ProcessContextReplacing<float> { block };
    inverseGain.process (context);
}
