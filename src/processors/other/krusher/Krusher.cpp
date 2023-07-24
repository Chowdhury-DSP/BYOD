#include "Krusher.h"
#include "processors/ParameterHelpers.h"

Krusher::Krusher (UndoManager* um)
    : BaseProcessor ("Krusher", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (sampleRateParam, vts, "sample_rate");
    loadParameterPointer (antialiasParam, vts, "antialias");
    loadParameterPointer (bitDepthParam, vts, "bit_depth");
    loadParameterPointer (brrFilterIndex, vts, "bit_reduction_filter");
    loadParameterPointer (mixParam, vts, "mix");

    uiOptions.backgroundColour = Colour { Colours::lightpink };
    uiOptions.powerColour = Colour { Colours::red.darker (0.2f) };
    uiOptions.info.description = "A lo-fi effect that reduces the sample rate and bit depth of the signal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    addPopupMenuParameter ("antialias");
}

ParamLayout Krusher::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    emplace_param<chowdsp::FloatParameter> (params,
                                            "sample_rate",
                                            "Downsample",
                                            createNormalisableRange (1000.0f, 48000.0f, 8000.0f),
                                            48000.0f,
                                            &freqValToString,
                                            &stringToFreqVal);
    emplace_param<chowdsp::BoolParameter> (params, "antialias", "Anti-Alias", false);
    emplace_param<chowdsp::FloatParameter> (params,
                                            "bit_depth",
                                            "Bits",
                                            NormalisableRange { 1.0f, 12.0f, 1.0f },
                                            12.0f,
                                            &floatValToString,
                                            &stringToFloatVal);
    emplace_param<chowdsp::ChoiceParameter> (params,
                                             "bit_reduction_filter",
                                             "Order",
                                             StringArray { "Zero-Order", "First-Order", "Second-Order", "Third-Order" },
                                             1);
    createPercentParameter (params, "mix", "Mix", 1.0f);

    return { params.begin(), params.end() };
}

static void setDryWetGain (chowdsp::Gain<float>& dryGain, chowdsp::Gain<float>& wetGain, float mix)
{
    const auto dryGainValue = std::sqrt (1.0f - mix);
    const auto wetGainValue = std::sqrt (mix);

    dryGain.setGainLinear (dryGainValue);
    wetGain.setGainLinear (wetGainValue);
}

void Krusher::prepare (double sampleRate, int samplesPerBlock)
{
    hostFs = (float) sampleRate;

    aaFilter.prepare (2);
    aiFilter.prepare (2);

    krusher_init_lofi_resample (&resample_state);

    for (auto& state : brFilterStates)
        state = {};

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (20.0f, (float) sampleRate);

    wetGain.setRampDurationSeconds (0.05);
    dryGain.setRampDurationSeconds (0.05);
    setDryWetGain (dryGain, wetGain, *mixParam);
    wetGain.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });
    dryGain.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });
    dryBuffer.setMaxSize (2, samplesPerBlock);
}

void Krusher::processDownsampler (const chowdsp::BufferView<float>& buffer, float targetFs, bool antialias) noexcept
{
    if (targetFs >= hostFs)
        return;

    aaFilter.calcCoefs ((float) targetFs * 0.48f, chowdsp::CoefficientCalculators::butterworthQ<float>, hostFs);
    aiFilter.calcCoefs ((float) targetFs * 0.48f, chowdsp::CoefficientCalculators::butterworthQ<float>, hostFs);

    if (antialias)
        aaFilter.processBlock (buffer);

    krusher_process_lofi_downsample (jai_context.get(),
                                     &resample_state,
                                     const_cast<float**> (buffer.getArrayOfWritePointers()),
                                     buffer.getNumChannels(),
                                     buffer.getNumSamples(),
                                     double (hostFs / targetFs));

    if (antialias)
        aiFilter.processBlock (buffer);
}

void Krusher::processAudio (AudioBuffer<float>& buffer)
{
    dryBuffer.setCurrentSize (buffer.getNumChannels(), buffer.getNumSamples());
    chowdsp::BufferMath::copyBufferData (buffer, dryBuffer);

    krusher_bit_reduce_process_block (const_cast<float**> (buffer.getArrayOfWritePointers()),
                                      buffer.getNumChannels(),
                                      buffer.getNumSamples(),
                                      brrFilterIndex->getIndex(),
                                      (int) *bitDepthParam,
                                      brFilterStates.data());

    processDownsampler (buffer, *sampleRateParam, antialiasParam->get());

    dcBlocker.processBlock (buffer);

    setDryWetGain (dryGain, wetGain, *mixParam);
    wetGain.process (buffer);
    dryGain.process (dryBuffer);
    chowdsp::BufferMath::addBufferData (dryBuffer, buffer);
}
