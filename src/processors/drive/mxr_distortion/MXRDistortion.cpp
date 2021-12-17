#include "MXRDistortion.h"

using namespace ParameterHelpers;

namespace
{
float paramSkew (float paramVal)
{
    return 1.0f - iLogPot (iLogPot (0.5f * paramVal + 0.5f));
}
} // namespace

MXRDistortion::MXRDistortion (UndoManager* um) : BaseProcessor ("MXR Distortion", createParameterLayout(), um)
{
    distParam = vts.getRawParameterValue ("dist");
    levelParam = vts.getRawParameterValue ("level");

    uiOptions.backgroundColour = Colours::teal;
    uiOptions.info.description = "Virtual analog emulation of the MXR Distortion+ overdrive pedal.";
    uiOptions.info.authors = StringArray { "Sam Schachter", "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout MXRDistortion::createParameterLayout()
{
    auto params = createBaseParams();
    createPercentParameter (params, "dist", "Distortion", 0.5f);
    createPercentParameter (params, "level", "Level", 0.5f);

    return { params.begin(), params.end() };
}

void MXRDistortion::prepare (double sampleRate, int samplesPerBlock)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        wdf[ch].prepare (sampleRate);
        wdf[ch].setParams (paramSkew (*distParam));
    }

    dcBlocker.prepare (sampleRate, samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    gain.prepare (spec);
    gain.setRampDurationSeconds (0.01);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 10000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void MXRDistortion::processAudio (AudioBuffer<float>& buffer)
{
    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    buffer.applyGain (2.5f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParams (paramSkew (*distParam));

        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            x[n] = wdf[ch].processSample (x[n]);
    }

    dcBlocker.processAudio (buffer);

    gain.setGainLinear (Decibels::decibelsToGain (-80.0f * (1.0f - iLogPot (*levelParam)), -80.0f));
    gain.process (context);
}
