#include "JuniorB.h"
#include "../../ParameterHelpers.h"

namespace
{
const String driveTag = "juniorb_drive";
const String blendTag = "juniorb_blend";
const String stagesTag = "juniorb_nstages";
} // namespace

JuniorB::JuniorB (UndoManager* um) : BaseProcessor ("Junior B", createParameterLayout(), um)
{
    using namespace chowdsp::ParamUtils;
    loadParameterPointer (driveParamPct, vts, driveTag);
    loadParameterPointer (blendParamPct, vts, blendTag);
    loadParameterPointer (stagesParam, vts, stagesTag);

    uiOptions.backgroundColour = Colours::slategrey.darker (0.2f);
    uiOptions.info.description = "Virtual analog emulation first stage from the Fender Pro-Junior Amplifier.";
    uiOptions.info.authors = StringArray { "Champ Darabundit", "Dirk Roosenburg", "Jatin Chowdhury" };
}

ParamLayout JuniorB::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, driveTag, "Tube Drive", 0.5f);
    createPercentParameter (params, blendTag, "Tube Blend", 1.0f);
    emplace_param<chowdsp::ChoiceParameter> (params, stagesTag, "Stages", StringArray { "1 Stage", "2 Stages", "3 Stages", "4 Stages" }, 1);

    return { params.begin(), params.end() };
}

void JuniorB::prepare (double sampleRate, int samplesPerBlock)
{
    const auto spec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };

    for (auto& stage : stages)
    {
        for (auto& wdf : stage.wdfs)
            wdf.prepare ((float) sampleRate);
    }

    for (auto* gain : { &driveGain, &wetGain, &dryGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.05);
    }

    dryBuffer.setMaxSize (2, samplesPerBlock);

    // pre-buffering
    ScopedValueSetter svs { preBuffering, true };
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 5000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void JuniorB::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    dryBuffer.setCurrentSize (numChannels, numSamples);
    chowdsp::BufferMath::copyBufferData (chowdsp::BufferView { buffer }, dryBuffer);

    const auto drivePercent = driveParamPct->getCurrentValue();
    const auto blendPercent = blendParamPct->getCurrentValue();
    const auto numStages = ! preBuffering ? stagesParam->getIndex() + 1 : maxNumStages;

    driveGain.setGainDecibels (drivePercent * 12.0f);
    driveGain.process (buffer);
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int stageIndex = 0; stageIndex < numStages; ++stageIndex)
        {
            auto& wdfStage = stages[stageIndex].wdfs[ch];
            for (int n = 0; n < numSamples; ++n)
                x[n] = wdfStage.process (x[n]);
        }
    }

    const auto dryGainLinear = std::sin (0.5f * MathConstants<float>::pi * (1.0f - blendPercent));
    const auto wetGainLinear = std::sin (0.5f * MathConstants<float>::pi * blendPercent);
    const auto polarityGain = numStages % 2 == 1 ? -1.0f : 1.0f;
    const auto makeupGainDB = (-15.0f + 9.0f * (1.0f - drivePercent)) * std::pow ((float) numStages, 0.2f);
    const auto makeupGain = polarityGain * juce::Decibels::decibelsToGain (makeupGainDB);
    wetGain.setGainLinear (wetGainLinear * makeupGain);
    dryGain.setGainLinear (dryGainLinear);

    wetGain.process (buffer);
    dryGain.process (dryBuffer);

    chowdsp::BufferMath::addBufferData (dryBuffer, buffer);
}
