#include "ShimmerReverb.h"
#include "processors/ParameterHelpers.h"

namespace
{
const String shiftTag = "shift";
const String sizeTag = "size";
const String feedbackTag = "feedback";
const String mixTag = "mix";
} // namespace

void ShimmerReverb::ShimmerFDNConfig::prepare (double sampleRate)
{
    shifter.prepare ({ sampleRate, 128, 1 });

    highCutFilter.prepare (numFDNChannels);
    highCutFilter.calcCoefs (6000.0f, (float) sampleRate);

    lowCutFilter.prepare (numFDNChannels);
    lowCutFilter.calcCoefs (50.0f, (float) sampleRate);

    Base::prepare (sampleRate);
}

void ShimmerReverb::ShimmerFDNConfig::reset()
{
    shifter.reset();
    highCutFilter.reset();
    lowCutFilter.reset();
    Base::reset();
}

const float* ShimmerReverb::ShimmerFDNConfig::doFeedbackProcess (ShimmerFDNConfig& fdnConfig, const float* data)
{
    auto filterChannel = [&fdnConfig] (float x, size_t channel)
    {
        x = fdnConfig.lowCutFilter.processSample (x, (int) channel);
        x = fdnConfig.highCutFilter.processSample (x, (int) channel);
        return x;
    };

    auto* fbData = fdnConfig.fbData.data();

    const auto shiftIndex = (size_t) numFDNChannels - 1;
    fbData[shiftIndex] = filterChannel (fdnConfig.shifter.processSample (0, data[shiftIndex]), shiftIndex);

    for (size_t i = 0; i < shiftIndex; ++i)
        fbData[i] = filterChannel (data[i], i);

    return Base::doFeedbackProcess (fdnConfig, fbData);
}

//==================================================
ShimmerReverb::ShimmerReverb (UndoManager* um) : BaseProcessor ("Shimmer Reverb", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    shiftParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, shiftTag));
    sizeParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, sizeTag));
    feedbackParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, feedbackTag));
    loadParameterPointer (mixParam, vts, mixTag);

    uiOptions.backgroundColour = Colours::lightgrey;
    uiOptions.powerColour = Colours::darksalmon.darker (0.2f);
    uiOptions.info.description = "A pitch-shifting reverb effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout ShimmerReverb::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    emplace_param<chowdsp::FloatParameter> (params,
                                            shiftTag,
                                            "Shift",
                                            NormalisableRange { -12.0f, 12.0f },
                                            -12.0f,
                                            &floatValToString,
                                            &stringToFloatVal);
    createPercentParameter (params, sizeTag, "Size", 1.0f);
    createPercentParameter (params, feedbackTag, "Decay", 1.0f);
    createPercentParameter (params, mixTag, "Mix", 0.5f);

    return { params.begin(), params.end() };
}

void ShimmerReverb::prepare (double sampleRate, int samplesPerBlock)
{
    shiftParam.prepare (sampleRate, samplesPerBlock);
    shiftParam.setRampLength (0.05);

    sizeParam.mappingFunction = [] (float x)
    {
        const auto delayMs = 50.0f * std::pow (250.0f / 50.0f, x);
        return delayMs;
    };
    sizeParam.setRampLength (0.2);
    sizeParam.prepare (sampleRate, samplesPerBlock);

    feedbackParam.mappingFunction = [] (float x)
    {
        const auto delayMs = 1000.0f * std::pow (10000.0f / 1000.0f, x);
        return delayMs;
    };
    feedbackParam.setRampLength (0.05);
    feedbackParam.prepare (sampleRate, samplesPerBlock);

    fdns = std::make_unique<std::array<chowdsp::Reverb::FDN<ShimmerFDNConfig>, 2>>();
    for (auto& channelFDN : *fdns)
        channelFDN.prepare (sampleRate);

    mixer.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });
    mixer.setMixingRule (dsp::DryWetMixingRule::sin3dB);

    for (int i = 0; i < numLFOs; ++i)
    {
        lfos[i].prepare ({ sampleRate, (uint32_t) samplesPerBlock, 1 });
        lfoVals[i] = 0;
    }
}

void ShimmerReverb::releaseMemory()
{
    fdns.reset();
}

void ShimmerReverb::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    shiftParam.process (numSamples);
    sizeParam.process (numSamples);
    feedbackParam.process (numSamples);

    mixer.pushDrySamples (dsp::AudioBlock<float> { buffer });
    mixer.setWetMixProportion (mixParam->getCurrentValue());

    static constexpr int smallBlockSize = 32;
    auto* x = buffer.getArrayOfWritePointers();
    const auto* shiftData = shiftParam.getSmoothedBuffer();
    const auto* sizeData = sizeParam.getSmoothedBuffer();
    const auto* feedbackData = feedbackParam.getSmoothedBuffer();

    auto& fdn = *fdns;
    for (int i = 0; i < numSamples;)
    {
        const auto smallBlockSamples = jmin (smallBlockSize, numSamples - i);

        for (float& lfoVal : lfoVals)
            lfoVal = lfoVal * 0.1f + 1.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            fdn[ch].setDelayTimeMsWithModulators<numLFOs> (sizeData[i], lfoVals);
            fdn[ch].getFDNConfig().setDecayTimeMs (fdn[ch], feedbackData[i], feedbackData[i] * 0.75f, 800.0f);
            fdn[ch].getFDNConfig().shifter.setShiftSemitones (shiftData[i]);

            for (int n = i; n < i + smallBlockSamples; ++n)
            {
                const auto input = x[ch][n];

                alignas (chowdsp::SIMDUtils::defaultSIMDAlignment) float fdnIn[numFDNChannels] {};
                std::fill (std::begin (fdnIn), std::end (fdnIn), input);
                const auto* fdnOut = fdn[ch].process (fdnIn);
                const auto output = std::accumulate (fdnOut, fdnOut + numFDNChannels, 0.0f) / (float) numFDNChannels;

                x[ch][n] = output;
            }
        }

        for (int j = 0; j < smallBlockSamples; ++j)
        {
            for (int k = 0; k < numLFOs; ++k)
                lfoVals[k] = lfos[k].processSample();
        }

        i += smallBlockSamples;
    }

    mixer.mixWetSamples (dsp::AudioBlock<float> { buffer });
}
