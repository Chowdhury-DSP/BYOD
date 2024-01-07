#include "FrequencyShifter.h"
#include "processors/ParameterHelpers.h"

FrequencyShifter::FrequencyShifter (UndoManager* um)
    : BaseProcessor ("Frequency Shifter", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (shiftParam, vts, "shift");

    uiOptions.backgroundColour = Colours::palevioletred.brighter (0.1f);
    uiOptions.powerColour = Colours::yellow;
    uiOptions.info.description = "A frequency shifter effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout FrequencyShifter::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createFreqParameter (params, "shift", "Shift", 0.0f, 200.0f, 20.0f, 20.0f);

    return { params.begin(), params.end() };
}

void FrequencyShifter::prepare (double sampleRate, int samplesPerBlock)
{
    hilbertFilter.reset();
    modulator.prepare ({ sampleRate, (uint32_t) samplesPerBlock,  1});
}

void FrequencyShifter::processAudio (AudioBuffer<float>& buffer)
{
    modulator.setFrequency (shiftParam->getCurrentValue());

    for (auto [ch, data] : chowdsp::buffer_iters::channels (buffer))
    {
        for (auto& x : data)
        {
            const auto [x_re, x_im] = hilbertFilter.process (x);
            const auto [mod_sin, mod_cos] = modulator.processSampleQuadrature();

            x = x_re * mod_sin - x_im * mod_cos;
        }
    }

    // TODO: DC blocker
}
