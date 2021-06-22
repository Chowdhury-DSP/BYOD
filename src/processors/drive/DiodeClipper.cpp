#include "DiodeClipper.h"
#include "../ParameterHelpers.h"

DiodeClipper::DiodeClipper (UndoManager* um) : BaseProcessor ("Diode Clipper", createParameterLayout(), um)
{
    cutoffParam = vts.getRawParameterValue ("cutoff");
    driveParam = vts.getRawParameterValue ("drive");

    // uiOptions.backgroundImage = Drawable::createFromImageData (BinaryData::DiodeClipper_png, BinaryData::DiodeClipper_pngSize);
    uiOptions.backgroundColour = Colours::white;
}

AudioProcessorValueTreeState::ParameterLayout DiodeClipper::createParameterLayout()
{
    using namespace ParameterHelpers;
    Params params;

    createFreqParameter (params, "cutoff", "Cutoff", 200.0f, 20.0e3f, 2000.0f, 5000.0f);
    createPercentParameter (params, "drive", "Drive", 0.5f);

    return { params.begin(), params.end() };
}

void DiodeClipper::prepare (double sampleRate, int samplesPerBlock)
{
    for (int ch = 0; ch < 2; ++ch)
        wdf[ch] = std::make_unique<DiodeClipperWDF> ((float) sampleRate);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto* gain : { &inGain, &outGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.01);
    }
}

void DiodeClipper::processAudio (AudioBuffer<float>& buffer)
{
    setGains (*driveParam);

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);
    inGain.process (context);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch]->setParameters (*cutoffParam);
        auto* x = buffer.getWritePointer (ch);
        wdf[ch]->process (x, buffer.getNumSamples());
    }

    outGain.process (context);
}

void DiodeClipper::setGains (float driveValue)
{
    auto inGainAmt = jmap (driveValue, 0.5f, 10.0f);
    auto outGainAmt = inGainAmt < 1.0f ? 1.0f / inGainAmt : 1.0f / std::sqrt (inGainAmt);

    inGain.setGainLinear (inGainAmt);
    outGain.setGainLinear (outGainAmt);
}
