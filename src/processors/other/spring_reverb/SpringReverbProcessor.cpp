#include "SpringReverbProcessor.h"
#include "../../ParameterHelpers.h"

SpringReverbProcessor::SpringReverbProcessor (UndoManager* um) : BaseProcessor ("Spring Reverb", createParameterLayout(), um)
{
    sizeParam = vts.getRawParameterValue ("size");
    decayParam = vts.getRawParameterValue ("decay");
    reflectParam = vts.getRawParameterValue ("reflect");
    spinParam = vts.getRawParameterValue ("spin");
    dampParam = vts.getRawParameterValue ("damping");
    chaosParam = vts.getRawParameterValue ("chaos");
    shakeParam = vts.getRawParameterValue ("shake");
    mixParam = vts.getRawParameterValue ("mix");

    uiOptions.backgroundColour = Colours::orange.darker (0.2f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "A reverb effect inspired by old spring reverb units.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout SpringReverbProcessor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createPercentParameter (params, "size", "Size", 0.5f);
    createPercentParameter (params, "decay", "Decay", 0.5f);
    createPercentParameter (params, "reflect", "Refl.", 0.5f);
    createPercentParameter (params, "spin", "Spin", 0.5f);
    createPercentParameter (params, "damping", "Damp", 0.5f);
    createPercentParameter (params, "chaos", "Chaos", 0.0f);
    createPercentParameter (params, "shake", "Shake", 0.0f);
    createPercentParameter (params, "mix", "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void SpringReverbProcessor::prepare (double sampleRate, int samplesPerBlock)
{
    reverb.prepare (sampleRate, samplesPerBlock);

    dryBuffer.setSize (2, samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    wetGain.prepare (spec);
    wetGain.setRampDurationSeconds (0.1);
}

void SpringReverbProcessor::processAudio (AudioBuffer<float>& buffer)
{
    reverb.setParams ({
                          sizeParam->load(),
                          decayParam->load(),
                          reflectParam->load(),
                          spinParam->load(),
                          dampParam->load(),
                          chaosParam->load(),
                          shakeParam->load() > 0.5f,
                      },
                      buffer.getNumSamples());

    dryBuffer.makeCopyOf (buffer);

    reverb.processBlock (buffer);

    dsp::AudioBlock<float> dryBlock (dryBuffer);
    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    wetGain.setGainLinear (mixParam->load());
    wetGain.process (context);

    block += dryBlock;
}
