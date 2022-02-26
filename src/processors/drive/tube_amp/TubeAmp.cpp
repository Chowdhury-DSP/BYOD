#include "TubeAmp.h"
#include "processors/ParameterHelpers.h"

TubeAmp::TubeAmp (UndoManager* um) : BaseProcessor ("Dirty Tube", createParameterLayout(), um)
{
    driveParam = vts.getRawParameterValue ("drive");

    uiOptions.backgroundColour = Colours::darkcyan;
    uiOptions.info.description = "Virtual analog model of an old tube amplifier.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout TubeAmp::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "drive", "Drive", 0.5f);

    return { params.begin(), params.end() };
}

void TubeAmp::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    for (auto* gain : { &gainIn, &gainOut })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.01f);
    }

    doubleBuffer.setSize (2, samplesPerBlock);

    for (auto& tubeProc : tube)
        tubeProc = std::make_unique<TubeProc> (sampleRate);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 5000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void TubeAmp::processAudio (AudioBuffer<float>& buffer)
{
    auto driveAmtSkew = std::pow (driveParam->load(), 8.0f);
    auto driveGain = 5.9f * driveAmtSkew + 0.1f;
    auto invDriveGain = 1.0f / (5.0f * driveAmtSkew + 1.0f);

    gainIn.setGainLinear (driveGain);
    gainOut.setGainLinear (invDriveGain);

    dsp::AudioBlock<float> block { buffer };
    dsp::ProcessContextReplacing<float> context { block };

    gainIn.process (context);

    doubleBuffer.makeCopyOf (buffer, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* x = doubleBuffer.getWritePointer (ch);
        tube[ch]->processBlock (x, buffer.getNumSamples());
    }

    buffer.makeCopyOf (doubleBuffer, true);

    gainOut.process (context);
}
