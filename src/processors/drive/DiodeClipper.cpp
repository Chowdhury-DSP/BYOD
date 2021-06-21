#include "DiodeClipper.h"

using namespace chowdsp::WDFT;

class DiodeClipperWDF
{
public:
    DiodeClipperWDF (float sampleRate) : C1 (capVal, sampleRate) {}

    void setParameters (float cutoff)
    {
        Vs.setResistanceValue (1.0f / (MathConstants<float>::twoPi * cutoff * capVal));
    }

    void process (float* buffer, const int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            Vs.setVoltage (buffer[n]);

            dp.incident (P1.reflected());
            buffer[n] = voltage<float> (C1);
            P1.incident (dp.reflected());
        }
    }

private:
    using wdf_type = float;
    using Res = ResistorT<wdf_type>;
    using Cap = CapacitorT<wdf_type>;
    using ResVs = ResistiveVoltageSourceT<wdf_type>;

    static constexpr float capVal = 47.0e-9f;

    ResVs Vs { 4700.0f };
    Cap C1;

    WDFParallelT<wdf_type, Cap, ResVs> P1 { C1, Vs };

    // GZ34 diode pair (@TODO implement other types of diodes)
    DiodePairT<wdf_type, decltype (P1)> dp { 2.52e-9f, 0.02585f, P1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeClipperWDF)
};

DiodeClipper::DiodeClipper (UndoManager* um) : BaseProcessor ("Diode Clipper", createParameterLayout(), um)
{
    cutoffParam = vts.getRawParameterValue ("cutoff");
    driveParam = vts.getRawParameterValue ("drive");
}

AudioProcessorValueTreeState::ParameterLayout DiodeClipper::createParameterLayout()
{
    using Params = std::vector<std::unique_ptr<RangedAudioParameter>>;
    Params params;

    NormalisableRange<float> cutoffRange { 200.0f, 20.0e3f };
    cutoffRange.setSkewForCentre (2000.0f);
    params.push_back (std::make_unique<AudioParameterFloat> ("cutoff", "Cutoff", cutoffRange, 5000.0f, "Hz"));

    params.push_back (std::make_unique<AudioParameterFloat> ("drive", "Drive", 0.0f, 1.0f, 0.5f));

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
