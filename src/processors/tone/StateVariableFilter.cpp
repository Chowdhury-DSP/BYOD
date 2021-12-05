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

AudioProcessorValueTreeState::ParameterLayout StateVariableFilter::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createFreqParameter (params, "freq", "Freq.", 20.0f, 20000.0f, 2000.0f, 8000.0f);

    NormalisableRange qRange { 0.2f, 5.0f };
    qRange.setSkewForCentre (0.7071f);
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
}

void StateVariableFilter::processAudio (AudioBuffer<float>& buffer)
{
    svf.setCutoffFrequency (*freqParam);
    svf.setResonance (*qParam);

    auto block = dsp::AudioBlock<float> { buffer };
    auto context = dsp::ProcessContextReplacing<float> { block };

    auto mode = (int) *modeParam;
    switch (mode)
    {
        case 0:
            svf.process<decltype (context), chowdsp::StateVariableFilterType::Lowpass> (context);
            break;
        case 1:
            svf.process<decltype (context), chowdsp::StateVariableFilterType::Highpass> (context);
            break;
        case 2:
            svf.process<decltype (context), chowdsp::StateVariableFilterType::Bandpass> (context);
            break;
    }
}
