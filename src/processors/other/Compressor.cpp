#include "Compressor.h"
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

Compressor::Compressor (UndoManager* um) : BaseProcessor ("Compressor", createParameterLayout(), um)
{
    threshDBParam = vts.getRawParameterValue ("thresh");
    ratioParam = vts.getRawParameterValue ("ratio");
    kneeDBParam = vts.getRawParameterValue ("knee");
    attackMsParam = vts.getRawParameterValue ("attack");
    releaseMsParam = vts.getRawParameterValue ("release");
    makeupDBParam = vts.getRawParameterValue ("makeup");

    gainComputer = std::make_unique<GainComputer>();

    uiOptions.backgroundColour = Colours::gold.darker (0.1f);
    uiOptions.powerColour = Colours::cyan.brighter (0.1f);
    uiOptions.paramIDsToSkip = { "knee", "makeup" };
    uiOptions.info.description = "A dynamic range compressor.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

Compressor::~Compressor() = default;

ParamLayout Compressor::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createGainDBParameter (params, "thresh", "Threshold", -30.0f, 6.0f, 0.0f);
    emplace_param<VTSParam> (
        params, "ratio", "Ratio", String(), createNormalisableRange (1.0f, 10.0f, 2.0f), 2.0f, [] (float val)
        { return String (val, 1) + " : 1"; },
        [] (const String& s)
        { return s.upToFirstOccurrenceOf (":", false, true).getFloatValue(); });
    createGainDBParameter (params, "knee", "Knee", 0.0f, 18.0f, 6.0f);

    emplace_param<VTSParam> (params, "attack", "Attack", String(), createNormalisableRange (1.0f, 100.0f, 10.0f), 10.0f, &timeMsValToString, &stringToTimeMsVal);
    emplace_param<VTSParam> (params, "release", "Release", String(), createNormalisableRange (10.0f, 1000.0f, 100.0f), 100.0f, &timeMsValToString, &stringToTimeMsVal);
    createGainDBParameter (params, "makeup", "Gain", -12.0f, 12.0f, 0.0f);

    return { params.begin(), params.end() };
}

void Compressor::prepare (double sampleRate, int samplesPerBlock)
{
    levelBuffer.setSize (1, samplesPerBlock);
    levelDetector.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });

    gainComputer->prepare (sampleRate, samplesPerBlock);

    makeupGain.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    makeupGain.setRampDurationSeconds (0.02);
}

void Compressor::processAudio (AudioBuffer<float>& buffer)
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
    levelDetector.setParameters (*attackMsParam, *releaseMsParam);
    levelDetector.process (dsp::ProcessContextReplacing<float> { levelBlock });

    gainComputer->setParameters (*threshDBParam, *ratioParam, *kneeDBParam);
    gainComputer->process (levelBlock);

    auto&& block = dsp::AudioBlock<float> { buffer };
    for (int ch = 0; ch < numChannels; ++ch)
        block.getSingleChannelBlock ((size_t) ch) *= gainComputer->gainBlock;

    makeupGain.setGainDecibels (*makeupDBParam);
    makeupGain.process (dsp::ProcessContextReplacing<float> { block });
}
