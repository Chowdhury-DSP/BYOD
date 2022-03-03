#include "StateVariableFilter.h"
#include "../ParameterHelpers.h"

StateVariableFilter::StateVariableFilter (UndoManager* um) : BaseProcessor ("SVF", createParameterLayout(), um)
{
    freqParam = vts.getRawParameterValue ("freq");
    qParam = vts.getRawParameterValue ("q_value");
    modeParam = vts.getRawParameterValue ("mode");

    uiOptions.backgroundColour = Colours::blanchedalmond;
    uiOptions.powerColour = Colours::red.darker (0.25f);
    uiOptions.info.description = "A state variable filter, with lowpass, highpass, and bandpass modes.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout StateVariableFilter::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createFreqParameter (params, "freq", "Freq.", 20.0f, 20000.0f, 2000.0f, 8000.0f);

    auto qRange = createNormalisableRange (0.2f, 5.0f, 0.7071f);
    params.push_back (std::make_unique<VTSParam> ("q_value",
                                                  "Q",
                                                  String(),
                                                  qRange,
                                                  0.7071f,
                                                  &floatValToString,
                                                  &stringToFloatVal));

    params.push_back (std::make_unique<AudioParameterChoice> ("mode",
                                                              "Mode",
                                                              StringArray { "LPF", "HPF", "BPF" },
                                                              0));

    return { params.begin(), params.end() };
}

void StateVariableFilter::prepare (double sampleRate, int samplesPerBlock)
{
    svf.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });

    freqSmooth.reset (sampleRate, 0.01);
    freqSmooth.setCurrentAndTargetValue (*freqParam);
}

template <chowdsp::StateVariableFilterType type>
void processFilter (AudioBuffer<float>& buffer, chowdsp::StateVariableFilter<float>& svf, StateVariableFilter::FreqSmooth& freqSmooth, float freqParam)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    freqSmooth.setTargetValue (freqParam);
    if (freqSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            auto* x = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                svf.setCutoffFrequency (freqSmooth.getNextValue());
                x[n] = svf.template processSample<type> (0, x[n]);
            }
        }
        else if (numChannels == 2)
        {
            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                svf.setCutoffFrequency (freqSmooth.getNextValue());
                left[n] = svf.template processSample<type> (0, left[n]);
                right[n] = svf.template processSample<type> (1, right[n]);
            }
        }
    }
    else
    {
        auto block = dsp::AudioBlock<float> { buffer };
        auto context = dsp::ProcessContextReplacing<float> { block };
        svf.setCutoffFrequency (freqSmooth.getNextValue());
        svf.process<decltype (context), type> (context);
    }
}

void StateVariableFilter::processAudio (AudioBuffer<float>& buffer)
{
    svf.setResonance (*qParam);

    auto mode = (int) *modeParam;
    switch (mode)
    {
        case 0:
            processFilter<chowdsp::StateVariableFilterType::Lowpass> (buffer, svf, freqSmooth, *freqParam);
            break;
        case 1:
            processFilter<chowdsp::StateVariableFilterType::Highpass> (buffer, svf, freqSmooth, *freqParam);
            break;
        case 2:
            processFilter<chowdsp::StateVariableFilterType::Bandpass> (buffer, svf, freqSmooth, *freqParam);
            break;
        default:
            break;
    }
}
