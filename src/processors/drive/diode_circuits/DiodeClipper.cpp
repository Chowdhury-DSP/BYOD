#include "DiodeClipper.h"
#include "../../ParameterHelpers.h"
#include "DiodeParameter.h"

DiodeClipper::DiodeClipper (UndoManager* um) : BaseProcessor ("Diode Clipper", createParameterLayout(), um)
{
    cutoffParam = vts.getRawParameterValue ("cutoff");
    driveParam = vts.getRawParameterValue ("drive");
    diodeTypeParam = vts.getRawParameterValue ("diode");
    nDiodesParam = vts.getRawParameterValue ("num_diodes");

    uiOptions.backgroundColour = Colours::white;
    uiOptions.info.description = "Emulation of a simple diode waveform clipper circuit with options for different configurations of diodes.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout DiodeClipper::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createFreqParameter (params, "cutoff", "Cutoff", 200.0f, 20.0e3f, 2000.0f, 5000.0f);
    createPercentParameter (params, "drive", "Drive", 0.5f);
    DiodeParameter::createDiodeParam (params, "diode");
    DiodeParameter::createNDiodesParam (params, "num_diodes");

    return { params.begin(), params.end() };
}

void DiodeClipper::prepare (double sampleRate, int samplesPerBlock)
{
    int diodeType = static_cast<int> (*diodeTypeParam);
    for (auto& wdfProc : wdf)
    {
        wdfProc.prepare ((float) sampleRate);
        wdfProc.setParameters (*cutoffParam, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam, true);
    }

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

    int diodeType = static_cast<int> (*diodeTypeParam);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch].setParameters (*cutoffParam, DiodeParameter::getDiodeIs (diodeType), *nDiodesParam);
        auto* x = buffer.getWritePointer (ch);
        wdf[ch].process (x, buffer.getNumSamples());
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
