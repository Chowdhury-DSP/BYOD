#include "Gate.h"
#include "../ParameterHelpers.h"

class Gate::GateEnvelope
{
public:
    GateEnvelope() = default;
    ~GateEnvelope() = default;

    void setParameters (float newThreshDB, float newAttackMs, float newHoldMs, float newReleaseMs)
    {
        detectorEnv.setParameters (0.01f, newAttackMs * 4.0f);
        detector01.setParameters (newAttackMs, newReleaseMs);
        threshGain = Decibels::decibelsToGain (newThreshDB);
        decayMs = newHoldMs;
    }

    void prepare (double sampleRate, int samplesPerBlock)
    {
        gainBuffer.setSize (1, samplesPerBlock);
        gainBlock = dsp::AudioBlock<float> { gainBuffer };

        detectorEnv.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });
        detector01.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

        state = Release;
        xOld = 0.0f;
        holdCounter = 0.0f;
        sampleTimeMs = 1000.0f / (float) sampleRate;
    }

    void process (const dsp::AudioBlock<float>& block) noexcept
    {
        const auto numSamples = block.getNumSamples();
        gainBlock = dsp::AudioBlock<float> (gainBuffer.getArrayOfWritePointers(), 1, numSamples);
        detectorEnv.process (dsp::ProcessContextNonReplacing<float> { block, gainBlock });

        auto* x = gainBlock.getChannelPointer (0);
        for (size_t n = 0; n < numSamples; ++n)
        {
            if (state == Release && x[n] > threshGain)
            {
                state = Attack;
                holdCounter = 0.0f;
            }

            if (state == Attack && x[n] < xOld)
                state = Hold;

            if (state == Hold)
                holdCounter += sampleTimeMs;

            if (state == Hold && x[n] < threshGain && holdCounter > decayMs)
                state = Release;

            xOld = x[n];
            switch (state)
            {
                case Attack:
                case Hold:
                    x[n] = detector01.processSample (1.0f);
                    break;
                case Release:
                    x[n] = detector01.processSample (0.0f);
                    break;
            }
        }
    }

    dsp::AudioBlock<float> gainBlock;

private:
    float threshGain = 0.0f;
    float decayMs = 0.0f;

    chowdsp::LevelDetector<float> detectorEnv;
    chowdsp::LevelDetector<float> detector01;
    float xOld;

    enum EnvState
    {
        Attack,
        Hold,
        Release
    };
    EnvState state;

    float holdCounter = 0.0f;
    float sampleTimeMs = 0.0001f;

    AudioBuffer<float> gainBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateEnvelope)
};

Gate::Gate (UndoManager* um) : BaseProcessor ("Gate", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (threshDBParam, vts, "thresh");
    loadParameterPointer (attackMsParam, vts, "attack");
    loadParameterPointer (holdMsParam, vts, "hold");
    loadParameterPointer (releaseMsParam, vts, "release");
    loadParameterPointer (makeupDBParam, vts, "makeup");

    gateEnvelope.emplace();

    uiOptions.backgroundColour = Colours::forestgreen.brighter (0.1f);
    uiOptions.powerColour = Colours::gold.brighter (0.1f);
    uiOptions.paramIDsToSkip = { "makeup" };
    uiOptions.info.description = "A simple noise gate.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

Gate::~Gate() = default;

ParamLayout Gate::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, "thresh", "Threshold", -60.0f, 0.0f, -35.0f);
    createTimeMsParameter (params, "attack", "Attack", createNormalisableRange (1.0f, 100.0f, 10.0f), 10.0f);
    createTimeMsParameter (params, "hold", "Hold", createNormalisableRange (10.0f, 1000.0f, 100.0f), 200.0f);
    createTimeMsParameter (params, "release", "Release", createNormalisableRange (10.0f, 1000.0f, 100.0f), 400.0f);
    createGainDBParameter (params, "makeup", "Gain", -12.0f, 12.0f, 0.0f);

    return { params.begin(), params.end() };
}

void Gate::prepare (double sampleRate, int samplesPerBlock)
{
    levelBuffer.setSize (1, samplesPerBlock);

    gateEnvelope->prepare (sampleRate, samplesPerBlock);

    makeupGain.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    makeupGain.setRampDurationSeconds (0.02);
}

void Gate::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    // set up level buffer
    if (numChannels == 1)
    {
        levelBuffer.makeCopyOf (buffer);
    }
    else
    {
        levelBuffer.setSize (1, numSamples, true, true, false);
        levelBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);

        for (int ch = 1; ch < numChannels; ++ch)
            levelBuffer.addFrom (0, 0, buffer, ch, 0, numSamples);

        levelBuffer.applyGain (1.0f / (float) numChannels);
    }

    auto&& levelBlock = dsp::AudioBlock<float> { levelBuffer };
    gateEnvelope->setParameters (*threshDBParam, *attackMsParam, *holdMsParam, *releaseMsParam);
    gateEnvelope->process (levelBlock);

    auto&& block = dsp::AudioBlock<float> { buffer };
    for (int ch = 0; ch < numChannels; ++ch)
        block.getSingleChannelBlock ((size_t) ch) *= gateEnvelope->gainBlock;

    makeupGain.setGainDecibels (*makeupDBParam);
    makeupGain.process (dsp::ProcessContextReplacing<float> { block });
}
