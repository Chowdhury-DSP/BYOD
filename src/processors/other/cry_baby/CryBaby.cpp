#include "CryBaby.h"
#include "CryBabyNDK.h"
#include "processors/ParameterHelpers.h"
#include "processors/netlist_helpers/CircuitQuantity.h"

namespace
{
const String controlFreqTag = "control_freq";

// this module needs some extra oversampling to help the Newton-Raphson solver converge
constexpr int oversampleRatio = 2;
} // namespace

CryBaby::CryBaby (UndoManager* um)
    : BaseProcessor ("Crying Child", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (controlFreqParam, vts, controlFreqTag);

    alphaSmooth.setRampLength (0.001);
    alphaSmooth.mappingFunction = [] (float x)
    { return juce::jmap (std::pow (x, 0.5f), 0.1f, 0.99f); };

    uiOptions.backgroundColour = Colours::whitesmoke.darker (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.2f);
    uiOptions.info.description = "\"Wah\" effect based on the Dunlop Cry Baby pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

CryBaby::~CryBaby() = default;

ParamLayout CryBaby::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, controlFreqTag, "Freq", 0.5f);
    return { params.begin(), params.end() };
}

void CryBaby::prepare (double sampleRate, int samplesPerBlock)
{
    alphaSmooth.prepare (sampleRate, samplesPerBlock);

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (30.0f, (float) sampleRate);

    needsOversampling = sampleRate < 88'200.0f;
    if (needsOversampling)
    {
        upsampler.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 }, oversampleRatio);
        downsampler.prepare ({ oversampleRatio * sampleRate, oversampleRatio * (uint32_t) samplesPerBlock, 2 }, oversampleRatio);
    }

    ndk_model = std::make_unique<CryBabyNDK>();
    ndk_model->prepare ((needsOversampling ? oversampleRatio : 1.0) * sampleRate, 0.5);

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 2000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void CryBaby::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    alphaSmooth.process (controlFreqParam->getCurrentValue(), numSamples);

    dcBlocker.processBlock (buffer);

    const auto processBlockNDK = [this] (const chowdsp::BufferView<float>& block, int smootherDivide = 1)
    {
        if (alphaSmooth.isSmoothing())
        {
            const auto alphaSmoothData = alphaSmooth.getSmoothedBuffer();
            for (int sample = 0; sample < block.getNumSamples();)
            {
                const auto samplesToProcess = juce::jmin (32, block.getNumSamples() - sample);

                const auto subBufferView = chowdsp::BufferView<float> { block, sample, samplesToProcess };

                ndk_model->set_alpha ((double) alphaSmoothData[sample / smootherDivide]);
                for (auto [ch, data] : chowdsp::buffer_iters::channels (subBufferView))
                    ndk_model->process_channel (data, ch);

                sample += samplesToProcess;
            }
        }
        else
        {
            ndk_model->set_alpha ((double) alphaSmooth.getCurrentValue());
            for (auto [ch, channelData] : chowdsp::buffer_iters::channels (block))
                ndk_model->process_channel (channelData, ch);
        }
    };

    if (needsOversampling)
    {
        const auto osBufferView = upsampler.process (buffer);
        processBlockNDK (osBufferView, oversampleRatio);
        downsampler.process (osBufferView, buffer);
    }
    else
    {
        processBlockNDK (buffer);
    }
}
