#include "BigMuffTone.h"
#include "../../ParameterHelpers.h"

namespace
{
const std::array<BigMuffTone::Components, 10> componentSets {
    BigMuffTone::Components { .name = "Triangle", .R8 = 22.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5 = 22.0e3f },
    BigMuffTone::Components { .name = "Ram's Head '73", .R8 = 33.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5 = 33.0e3f },
    BigMuffTone::Components { .name = "Ram's Head '75", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 4.0e-9f, .R5 = 22.0e3f },
    BigMuffTone::Components { .name = "Russian", .R8 = 20.0e3f, .C8 = 10.0e-9f, .C9 = 3.9e-9f, .R5 = 22.0e3f },
    BigMuffTone::Components { .name = "Hoof Fuzz", .R8 = 39.0e3f, .C8 = 6.8e-9f, .C9 = 6.8e-9f, .R5_1 = 2.2e3f, .R5_2 = 25.0e3f },
    BigMuffTone::Components { .name = "AMZ v1", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 12.0e-9f, .R5_1 = 3.3e3f, .R5_2 = 25.0e3f },
    BigMuffTone::Components { .name = "AMZ v2", .R8 = 470.0e3f, .C8 = 1.5e-9f, .C9 = 15.0e-9f, .R5_1 = 3.3e3f, .R5_2 = 25.0e3f, .RT = 250.0e3f },
    BigMuffTone::Components { .name = "Supercollider", .R8 = 20.0e3f, .C8 = 10.0e-9f, .C9 = 3.9e-9f, .R5_1 = 10.0e3f, .R5_2 = 25.0e3f },
    BigMuffTone::Components { .name = "Flat Mids", .R8 = 39.0e3f, .C8 = 10.0e-9f, .C9 = 10.0e-9f, .R5 = 22.0e3f },
    BigMuffTone::Components { .name = "Bump Mids", .R8 = 39.0e3f, .C8 = 5.6e-9f, .C9 = 10.0e-9f, .R5 = 22.0e3f },
};
}

BigMuffTone::BigMuffTone (UndoManager* um) : BaseProcessor ("Big Muff Tone", createParameterLayout(), um),
                                             comps (&componentSets[2])
{
    toneParam = vts.getRawParameterValue ("tone");
    midsParam = vts.getRawParameterValue ("mids");
    typeParam = vts.getRawParameterValue ("type");

    uiOptions.backgroundColour = Colours::darkred.brighter (0.3f);
    uiOptions.powerColour = Colours::lime.darker (0.1f);
    uiOptions.info.description = "Emulation of the tone stage from various Big Muff-style pedals.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout BigMuffTone::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, "tone", "Tone", 0.5f);
    createPercentParameter (params, "mids", "Mids", 0.5f);

    StringArray types;
    for (auto& set : componentSets)
        types.add (set.name);
    emplace_param<AudioParameterChoice> (params, "type", "Type", types, 2);

    return { params.begin(), params.end() };
}

void BigMuffTone::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    fs = (float) sampleRate;

    comps = &componentSets[(int) *typeParam];
    toneSmooth.reset (sampleRate, 0.01);
    toneSmooth.setCurrentAndTargetValue (*toneParam);
    midsSmooth.reset (sampleRate, 0.01);
    midsSmooth.setCurrentAndTargetValue (*midsParam);
    calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), *comps);

    for (auto& filt : iir)
        filt.reset();
}

void BigMuffTone::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    comps = &componentSets[(int) *typeParam];
    toneSmooth.setTargetValue (*toneParam);
    midsSmooth.setTargetValue (*midsParam);
    auto** x = buffer.getArrayOfWritePointers();

    if (toneSmooth.isSmoothing() || midsSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), *comps);
                x[0][n] = iir[0].processSample (x[0][n]);
            }
        }
        else if (numChannels == 2)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), *comps);
                x[0][n] = iir[0].processSample (x[0][n]);
                x[1][n] = iir[1].processSample (x[1][n]);
            }
        }
    }
    else
    {
        calcCoefs (toneSmooth.getNextValue(), midsSmooth.getNextValue(), *comps);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < numSamples; ++n)
                x[ch][n] = iir[ch].processSample (x[ch][n]);
        }
    }

    buffer.applyGain (Decibels::decibelsToGain (6.0f));
}
