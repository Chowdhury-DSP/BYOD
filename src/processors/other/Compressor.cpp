#include "Compressor.h"
#include "../BufferHelpers.h"
#include "../ParameterHelpers.h"

class Compressor::GainComputer
{
public:
    GainComputer() = default;
    ~GainComputer() = default;

    void setParameters (float newThreshDB, float newRatio, float newKneeDB)
    {
        threshSmooth.setTargetValue (Decibels::decibelsToGain (newThreshDB));
        ratioSmooth.setTargetValue (newRatio);
        kneeDB = newKneeDB;
    }

    void prepare (double sampleRate, int samplesPerBlock)
    {
        gainBuffer.setSize (1, samplesPerBlock);
        gainBlock = dsp::AudioBlock<float> { gainBuffer };

        threshSmooth.reset (sampleRate, 0.05);
        ratioSmooth.reset (sampleRate, 0.05);
    }

    void process (const dsp::AudioBlock<float>& block) noexcept
    {
        const auto threshDB = Decibels::gainToDecibels (threshSmooth.getCurrentValue());
        const auto kneeLower = Decibels::decibelsToGain (threshDB - kneeDB / 2.0f);
        const auto kneeUpper = Decibels::decibelsToGain (threshDB + kneeDB / 2.0f);

        auto* levelPtr = block.getChannelPointer (0);
        auto* gainPtr = gainBlock.getChannelPointer (0);

        const auto numSamples = block.getNumSamples();
        for (size_t n = 0; n < numSamples; ++n)
            gainPtr[n] = calcGain (levelPtr[n], kneeLower, kneeUpper);
    }

    dsp::AudioBlock<float> gainBlock;

private:
    inline float calcGain (float level, float kneeLower, float kneeUpper)
    {
        auto curThresh = threshSmooth.getNextValue();
        auto curRatio = ratioSmooth.getNextValue();

        auto xAbs = std::abs (level);
        if (xAbs <= kneeLower) // below thresh
            return 1.0f;

        if (xAbs >= kneeUpper) // compression range
            return std::pow (level / curThresh, (1.0f / curRatio) - 1.0f);

        // knee range
        auto gainCorr = Decibels::gainToDecibels (xAbs / curThresh) + 0.5f * kneeDB;
        auto aFF = (1.0f - (1.0f / curRatio)) / (2.0f * kneeDB);
        auto gainDB = -1.0f * aFF * gainCorr * gainCorr;
        return Decibels::decibelsToGain (gainDB);
    }

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> threshSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> ratioSmooth;

    float kneeDB = 1.0f;

    AudioBuffer<float> gainBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainComputer)
};

Compressor::Compressor (UndoManager* um) : BaseProcessor (
    "Compressor",
    createParameterLayout(),
    InputPort {},
    OutputPort {},
    um,
    [] (InputPort port)
    {
        if (port == InputPort::LevelInput)
            return PortType::level;
        return PortType::audio;
    },
    [] (OutputPort port)
    {
        if (port == OutputPort::LevelOutput)
            return PortType::level;
        return PortType::audio;
    })
{
    using namespace ParameterHelpers;
    loadParameterPointer (threshDBParam, vts, "thresh");
    loadParameterPointer (ratioParam, vts, "ratio");
    loadParameterPointer (kneeDBParam, vts, "knee");
    loadParameterPointer (attackMsParam, vts, "attack");
    loadParameterPointer (releaseMsParam, vts, "release");
    loadParameterPointer (makeupDBParam, vts, "makeup");

    gainComputer = std::make_unique<GainComputer>();

    uiOptions.backgroundColour = Colours::gold.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.paramIDsToSkip = { "knee", "makeup" };
    uiOptions.info.description = "A dynamic range compressor.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    disableWhenInputConnected ({ "attack", "release" }, LevelInput);
}

Compressor::~Compressor() = default;

ParamLayout Compressor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, "thresh", "Threshold", -30.0f, 6.0f, 0.0f);
    createRatioParameter (params, "ratio", "Ratio", createNormalisableRange (1.0f, 10.0f, 2.0f), 2.0f);
    createGainDBParameter (params, "knee", "Knee", 0.0f, 18.0f, 6.0f);

    createTimeMsParameter (params, "attack", "Attack", createNormalisableRange (1.0f, 100.0f, 10.0f), 10.0f);
    createTimeMsParameter (params, "release", "Release", createNormalisableRange (10.0f, 1000.0f, 100.0f), 100.0f);
    createGainDBParameter (params, "makeup", "Gain", -12.0f, 12.0f, 0.0f);

    return { params.begin(), params.end() };
}

void Compressor::prepare (double sampleRate, int samplesPerBlock)
{
    audioOutBuffer.setSize (2, samplesPerBlock);
    levelOutBuffer.setSize (1, samplesPerBlock);
    levelDetector.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

    gainComputer->prepare (sampleRate, samplesPerBlock);

    makeupGain.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    makeupGain.setRampDurationSeconds (0.02);
}

void Compressor::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    levelOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (LevelInput))
    {
        BufferHelpers::collapseToMonoBuffer (getInputBuffer (LevelInput), levelOutBuffer);
        FloatVectorOperations::clip (levelOutBuffer.getWritePointer (0),
                                     levelOutBuffer.getReadPointer (0),
                                     0.0f,
                                     10.0f,
                                     numSamples);
    }
    else
    {
        if (inputsConnected.contains (AudioInput))
        {
            levelDetector.setParameters (*attackMsParam, *releaseMsParam);
            levelDetector.processBlock (getInputBuffer (AudioInput), levelOutBuffer);
        }
        else
        {
            levelOutBuffer.clear();
        }
    }

    if (inputsConnected.contains (AudioInput))
    {
        gainComputer->setParameters (*threshDBParam, *ratioParam, *kneeDBParam);
        gainComputer->process (levelOutBuffer);

        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            FloatVectorOperations::multiply (audioOutBuffer.getWritePointer (ch), audioInBuffer.getReadPointer (ch), gainComputer->gainBlock.getChannelPointer (0), numSamples);
        }

        auto&& audioOutBlock = dsp::AudioBlock<float> { audioOutBuffer };
        makeupGain.setGainDecibels (*makeupDBParam);
        makeupGain.process (dsp::ProcessContextReplacing<float> { audioOutBlock });
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
}

void Compressor::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    levelOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (LevelInput)) //make mono and pass samples through
    {
        const auto& levelInputBuffer = getInputBuffer (LevelInput);
        BufferHelpers::collapseToMonoBuffer (levelInputBuffer, levelOutBuffer);
    }
    else
    {
        levelOutBuffer.clear();
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        for (int ch = 0; ch < numChannels; ++ch)
            audioOutBuffer.copyFrom (ch, 0, audioInBuffer, ch % numChannels, 0, numSamples);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }
    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (LevelOutput) = &levelOutBuffer;
}
