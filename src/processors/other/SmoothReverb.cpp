#include "SmoothReverb.h"
#include "../ParameterHelpers.h"

namespace SmoothReverbTags
{
const String decayTag = "decay";
const String relaxTag = "relax";
const String lowCutTag = "low_cut";
const String highCutTag = "high_cut";
const String mixTag = "mix";

constexpr auto preDelay1LengthMs = 43.0f;
constexpr auto preDelay2LengthMs = 77.0f;

constexpr auto preDelay1CutoffHz = 3000.0f;
constexpr auto preDelay2CutoffHz = 2000.0f;
} // namespace SmoothReverbTags

SmoothReverb::SmoothReverb (UndoManager* um) : BaseProcessor ("Smooth Reverb", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (decayMsParam, vts, SmoothReverbTags::decayTag);
    loadParameterPointer (relaxParam, vts, SmoothReverbTags::relaxTag);
    loadParameterPointer (lowCutHzParam, vts, SmoothReverbTags::lowCutTag);
    loadParameterPointer (highCutHzParam, vts, SmoothReverbTags::highCutTag);
    loadParameterPointer (mixPctParam, vts, SmoothReverbTags::mixTag);

    uiOptions.backgroundColour = Colour { 0xFF8BBBD5 };
    uiOptions.powerColour = Colour { 0xFFCC4514 };
    uiOptions.info.description = "A smooth reverb effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout SmoothReverb::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createTimeMsParameter (params, SmoothReverbTags::decayTag, "Decay", createNormalisableRange (500.0f, 5000.0f, 1500.0f), 1500.0f);
    createPercentParameter (params, SmoothReverbTags::relaxTag, "Relax", 0.5f);

    // @TODO: can we figure out a way to combine these two parameters as one slider on the UI?
    createFreqParameter (params, SmoothReverbTags::lowCutTag, "Low Cut", 20.0f, 2000.0f, 200.0f, 200.0f);
    createFreqParameter (params, SmoothReverbTags::highCutTag, "High Cut", 500.0f, 20000.0f, 8000.0f, 8000.0f);

    createPercentParameter (params, SmoothReverbTags::mixTag, "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void SmoothReverb::prepare (double sampleRate, int samplesPerBlock)
{
    auto spec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };

    preDelay1.prepare (spec);
    preDelay2.prepare (spec);
    preDelayFilt.prepare (spec);

    float preDelayCutoffHzVec alignas (16)[] = { SmoothReverbTags::preDelay1CutoffHz, SmoothReverbTags::preDelay2CutoffHz, 0.0f, 0.0f };
    preDelayFilt.setCutoffFrequency (xsimd::load_aligned (preDelayCutoffHzVec));

    fs = (float) sampleRate;
    preDelay1.setDelay (SmoothReverbTags::preDelay1LengthMs * 0.001f * fs);
    preDelay2.setDelay (SmoothReverbTags::preDelay2LengthMs * 0.001f * fs);

    reverbInternal = std::make_unique<ReverbInternal>();
    reverbInternal->diffuser.prepare (sampleRate);
    reverbInternal->fdn.prepare (sampleRate);

    envelopeFollower.prepare (spec);
    envelopeFollower.setParameters (20.0f, 2000.0f);

    lowCutFilter.prepare (spec);
    lowCutFilter.setCutoffFrequency (*lowCutHzParam);
    highCutFilter.prepare (spec);
    highCutFilter.setCutoffFrequency (*highCutHzParam);

    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::sin3dB);

    outBuffer.setSize (2, samplesPerBlock);
}

void SmoothReverb::releaseMemory()
{
    preDelay1.free();
    preDelay2.free();
    reverbInternal.reset();
    outBuffer.setSize (0, 0);
}

void SmoothReverb::processReverb (float* left, float* right, int numSamples)
{
    float curLevel = 0.0f;
    for (int n = 0; n < numSamples; ++n)
        curLevel = envelopeFollower.processSample (chowdsp::Power::ipow<2> (left[n] + right[n]));

    const auto curDecayParam = decayMsParam->getCurrentValue();
    const auto modFactor = 2.5f * std::pow (curDecayParam / 5000.0f, 1.25f);
    const auto delayFactor = 1.0f + (modFactor * *relaxParam) * curLevel;
    const auto baseDelay1 = SmoothReverbTags::preDelay1LengthMs * 0.001f * fs;
    const auto baseDelay2 = SmoothReverbTags::preDelay2LengthMs * 0.001f * fs;

    preDelay1.setDelay (baseDelay1 * delayFactor);
    preDelay2.setDelay (baseDelay2 * delayFactor);

    auto& diffuser = reverbInternal->diffuser;
    auto& fdn = reverbInternal->fdn;
    diffuser.setDiffusionTimeMs (std::pow (curDecayParam * 0.005f, 0.75f));
    fdn.setDelayTimeMs (std::pow (curDecayParam * 0.2f, 0.95f));
    fdn.getFDNConfig().setDecayTimeMs (reverbInternal->fdn, curDecayParam * 1.25f, curDecayParam * 0.5f, 750.0f);

    float xVecArr alignas (16)[4] {};
    for (int n = 0; n < numSamples; ++n)
    {
        xVecArr[0] = preDelay1.popSample (0);
        xVecArr[1] = preDelay2.popSample (0);
        auto xVec = xsimd::load_aligned (xVecArr);

        xVec = preDelayFilt.processSample (0, xVec);
        xVec.store_aligned (xVecArr);

        auto y1 = xVecArr[0];
        auto y2 = xVecArr[1];

        const auto left_tanh = dsp::FastMathApproximations::tanh (left[n]);
        const auto right_tanh = dsp::FastMathApproximations::tanh (right[n]);
        preDelay1.pushSample (0, left_tanh);
        preDelay2.pushSample (0, right_tanh);

        float diffuserInVec alignas (16)[nDiffuserChannels] {};
        std::fill (diffuserInVec, diffuserInVec + nDiffuserChannels / 2, left_tanh);
        std::fill (diffuserInVec + nDiffuserChannels / 2, diffuserInVec + nDiffuserChannels, right_tanh);
        const auto y3 = diffuser.process (diffuserInVec);

        float fdnInVec alignas (16)[nFDNChannels] { 0.5f * y1, 0.5f * y2, left_tanh, right_tanh };
        for (int i = 4; i < nFDNChannels; ++i)
            fdnInVec[i] = 0.25f * y3[i % nDiffuserChannels];

        static constexpr auto reflectionsMix = 0.2f;
        static constexpr auto diffusionMix = 0.3f;
        static constexpr auto fdnMix = 0.45f;

        const auto y3Left = y3[0];
        const auto y3Right = y3[1];

        const auto fdnOut = fdn.process (fdnInVec);
        float yFDNLeft = 0.0f, yFDNRight = 0.0f;
        for (int i = 0; i < nFDNChannels; i += 2)
        {
            yFDNLeft += fdnOut[i];
            yFDNRight += fdnOut[i + 1];
        }

        left[n] = reflectionsMix * y1 + diffusionMix * y3Left + fdnMix * yFDNLeft;
        right[n] = reflectionsMix * y2 + diffusionMix * y3Right + fdnMix * yFDNRight;
    }
}

void SmoothReverb::processAudio (AudioBuffer<float>& buffer)
{
    const auto numInChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    outBuffer.setSize (2, numSamples);
    for (int ch = 0; ch < 2; ++ch)
        outBuffer.copyFrom (ch, 0, buffer, ch % numInChannels, 0, numSamples);

    // push dry signal
    mixer.setWetMixProportion (*mixPctParam);
    mixer.pushDrySamples (dsp::AudioBlock<float> { outBuffer });

    // process reverb
    static constexpr int smallBufferSize = 32;
    for (int i = 0; i < numSamples;)
    {
        const auto samplesToProcess = jmin (smallBufferSize, numSamples - i);
        processReverb (outBuffer.getWritePointer (0) + i, outBuffer.getWritePointer (1) + i, samplesToProcess);
        i += samplesToProcess;
    }

    // filters
    lowCutFilter.setCutoffFrequency (*lowCutHzParam);
    lowCutFilter.processBlock (outBuffer);
    highCutFilter.setCutoffFrequency (*highCutHzParam);
    highCutFilter.processBlock (outBuffer);

    // mix dry/wet
    mixer.mixWetSamples (dsp::AudioBlock<float> { outBuffer });

    outputBuffers.getReference (0) = &outBuffer;
}

void SmoothReverb::processAudioBypassed (AudioBuffer<float>& buffer)
{
    outBuffer.makeCopyOf (buffer, true);
    outputBuffers.getReference (0) = &outBuffer;
}
