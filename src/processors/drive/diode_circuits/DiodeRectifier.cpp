#include "DiodeRectifier.h"
#include "../../ParameterHelpers.h"

DiodeRectifier::DiodeRectifier (UndoManager* um) : BaseProcessor ("Diode Rectifier", createParameterLayout(), um)
{
    cutoffParam = vts.getRawParameterValue ("cutoff");
    driveParam = vts.getRawParameterValue ("drive");
    diodeTypeParam = vts.getRawParameterValue ("diode");
    nDiodesParam = vts.getRawParameterValue ("num_diodes");

    uiOptions.backgroundColour = Colours::goldenrod.brighter (0.25f);
}

AudioProcessorValueTreeState::ParameterLayout DiodeRectifier::createParameterLayout()
{
    using namespace ParameterHelpers;
    
    auto params = createBaseParams();
    createFreqParameter (params, "cutoff", "Cutoff", 200.0f, 20.0e3f, 2000.0f, 5000.0f);
    createPercentParameter (params, "drive", "Drive", 0.5f);

    params.push_back (std::make_unique<AudioParameterChoice> ("diode",
                                                              "Diodes",
                                                              StringArray { "GZ34", "1N34", "1N4148" },
                                                              0));

    NormalisableRange<float> nDiodesRange { 0.3f, 3.0f };
    nDiodesRange.setSkewForCentre (1.0f);
    params.push_back (std::make_unique<VTSParam> ("num_diodes",
                                                  "# Diodes",
                                                  String(),
                                                  nDiodesRange,
                                                  1.0f,
                                                  &floatValToString,
                                                  &stringToFloatVal));

    return { params.begin(), params.end() };
}

void DiodeRectifier::prepare (double sampleRate, int samplesPerBlock)
{
    for (int ch = 0; ch < 2; ++ch)
        wdf[ch] = std::make_unique<DiodeRectifierWDF> ((float) sampleRate);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto* gain : { &inGain, &outGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.01);
    }
}

void DiodeRectifier::processAudio (AudioBuffer<float>& buffer)
{
    setGains (*driveParam);

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);
    inGain.process (context);

    int diodeType = static_cast<int> (*diodeTypeParam);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        wdf[ch]->setParameters (*cutoffParam, DiodeRectifierWDF::getDiodeIs (diodeType), *nDiodesParam);
        auto* x = buffer.getWritePointer (ch);
        wdf[ch]->process (x, buffer.getNumSamples());
    }

    outGain.process (context);
}

void DiodeRectifier::setGains (float driveValue)
{
    auto inGainAmt = jmap (driveValue, 0.5f, 10.0f);
    auto outGainAmt = inGainAmt < 1.0f ? 1.0f / inGainAmt : 1.0f / std::sqrt (inGainAmt);

    inGain.setGainLinear (inGainAmt);
    outGain.setGainLinear (-1.0f * outGainAmt); // flip rectifier polarity so output wave is positive
}
