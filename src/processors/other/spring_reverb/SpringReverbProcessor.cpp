#include "SpringReverbProcessor.h"
#include "../../ParameterHelpers.h"
#include "SpringReverb.h"

SpringReverbProcessor::SpringReverbProcessor (UndoManager* um) : BaseProcessor ("Spring Reverb", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (sizeParam, vts, "size");
    loadParameterPointer (decayParam, vts, "decay");
    loadParameterPointer (reflectParam, vts, "reflect");
    loadParameterPointer (spinParam, vts, "spin");
    loadParameterPointer (dampParam, vts, "damping");
    loadParameterPointer (chaosParam, vts, "chaos");
    loadParameterPointer (shakeParam, vts, "shake");
    loadParameterPointer (mixParam, vts, "mix");

    uiOptions.backgroundColour = Colours::orange.darker (0.2f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.info.description = "A reverb effect inspired by old spring reverb units.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

SpringReverbProcessor::~SpringReverbProcessor() = default;

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
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    reverb = std::make_unique<SpringReverb> (sampleRate);
    reverb->prepare (spec);

    dryBuffer.setSize (2, samplesPerBlock);

    wetGain.prepare (spec);
    wetGain.setRampDurationSeconds (0.1);
}

void SpringReverbProcessor::releaseMemory()
{
    reverb.reset();
}

void SpringReverbProcessor::processAudio (AudioBuffer<float>& buffer)
{
    if (numChannels != buffer.getNumChannels())
    {
        reverb->reset();
        numChannels = buffer.getNumChannels();
    }

    reverb->setParams ({
        sizeParam->getCurrentValue(),
        decayParam->getCurrentValue(),
        reflectParam->getCurrentValue(),
        spinParam->getCurrentValue(),
        dampParam->getCurrentValue(),
        chaosParam->getCurrentValue(),
        shakeParam->getCurrentValue() > 0.5f,
    });

    dryBuffer.makeCopyOf (buffer);

    reverb->processBlock (buffer);

    dsp::AudioBlock<float> dryBlock (dryBuffer);
    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    wetGain.setGainLinear (mixParam->getCurrentValue());
    wetGain.process (context);

    block += dryBlock;
}
