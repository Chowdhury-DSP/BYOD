#include "Chorus.h"

namespace
{
constexpr float rate1Low = 0.005f;
constexpr float rate1High = 5.0f;
constexpr float rate2Low = 0.5f;
constexpr float rate2High = 40.0f;

constexpr float delay1Ms = 0.6f;
constexpr float delay2Ms = 0.2f;

const String delayTypeTag = "delay_type";
} // namespace

Chorus::Chorus (UndoManager* um) : BaseProcessor ("Chorus", createParameterLayout(), um)
{
    rateParam = vts.getRawParameterValue ("rate");
    depthParam = vts.getRawParameterValue ("depth");
    fbParam = vts.getRawParameterValue ("feedback");
    mixParam = vts.getRawParameterValue ("mix");
    delayTypeParam = vts.getRawParameterValue (delayTypeTag);

    uiOptions.backgroundColour = Colours::purple.brighter (0.25f);
    uiOptions.powerColour = Colours::yellow.brighter (0.1f);
    uiOptions.paramIDsToSkip = { delayTypeTag };
    uiOptions.info.description = "A chorus effect using BBD-emulated delay lines. Note that this effect works best in stereo.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

AudioProcessorValueTreeState::ParameterLayout Chorus::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createPercentParameter (params, "rate", "Rate", 0.5f);
    createPercentParameter (params, "depth", "Depth", 0.5f);
    createPercentParameter (params, "feedback", "Feedback", 0.0f);
    createPercentParameter (params, "mix", "Mix", 0.5f);

    emplace_param<AudioParameterChoice> (params, delayTypeTag, "Delay Type", StringArray { "Clean", "Lo-Fi" }, 0);

    return { params.begin(), params.end() };
}

void Chorus::prepare (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    fs = (float) sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < delaysPerChannel; ++i)
        {
            cleanDelay[ch][i].prepare (monoSpec);
            lofiDelay[ch][i].prepare (monoSpec);

            slowLFOs[ch][i].prepare (monoSpec);
            fastLFOs[ch][i].prepare (monoSpec);
        }

        slowSmooth[ch].reset (sampleRate, 0.05);
        fastSmooth[ch].reset (sampleRate, 0.05);

        feedbackState[ch] = 0.0f;
        fbSmooth[ch].reset (sampleRate, 0.01);
    }

    // set phase offsets
    constexpr float piOver3 = MathConstants<float>::pi / 3.0f;
    slowLFOs[0][0].reset (-piOver3);
    fastLFOs[0][0].reset (-piOver3);
    slowLFOs[1][1].reset (piOver3);
    fastLFOs[1][1].reset (piOver3);

    aaFilter.prepare (spec);
    aaFilter.setCutoffFrequency (12000.0f);

    dryWetMixer.prepare (spec);

    dcBlocker.prepare (spec);
    dcBlocker.setCutoffFrequency (60.0f);

    stereoBuffer.setSize (2, samplesPerBlock);
}

template <typename DelayArrType>
void Chorus::processChorus (AudioBuffer<float>& buffer, DelayArrType& delay)
{
    const auto numSamples = buffer.getNumSamples();

    auto slowRate = rate1Low * std::pow (rate1High / rate1Low, *rateParam);
    auto fastRate = rate2Low * std::pow (rate2High / rate2Low, *rateParam);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < delaysPerChannel; ++i)
        {
            slowLFOs[ch][i].setFrequency (slowRate);
            fastLFOs[ch][i].setFrequency (fastRate);
            delay[ch][i].setFilterFreq (10000.0f);
        }

        auto fbAmount = std::sqrt (fbParam->load());
        if constexpr (std::is_same_v<DelayArrType, decltype (lofiDelay)>)
            fbAmount *= 0.4f;
        else
            fbAmount *= 0.5f;

        fbSmooth[ch].setTargetValue (fbAmount);
        slowSmooth[ch].setTargetValue (delay1Ms * 0.001f * fs * *depthParam);
        fastSmooth[ch].setTargetValue (delay2Ms * 0.001f * fs * *depthParam);

        auto* x = buffer.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
        {
            auto xIn = std::tanh (x[n] * 0.75f - feedbackState[ch]);

            auto slowSmoothMult = slowSmooth[ch].getNextValue();
            auto fastSmoothMult = fastSmooth[ch].getNextValue();

            x[n] = 0.0f;
            for (int i = 0; i < delaysPerChannel; ++i)
            {
                float delayAmt = slowSmoothMult * (1.0f + 0.95f * slowLFOs[ch][i].processSample());
                delayAmt += fastSmoothMult * (1.0f + 0.95f * fastLFOs[ch][i].processSample());

                delay[ch][i].setDelay (delayAmt);
                delay[ch][i].pushSample (0, xIn);
                x[n] += delay[ch][i].popSample (0);
            }

            x[n] = aaFilter.processSample<chowdsp::StateVariableFilterType::Lowpass> (ch, x[n]);

            feedbackState[ch] = fbSmooth[ch].getNextValue() * x[n];
            feedbackState[ch] = dcBlocker.processSample<chowdsp::StateVariableFilterType::Highpass> (ch, feedbackState[ch]);
        }
    }
}

void Chorus::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    // always have a stereo output!
    auto& processBuffer = buffer;
    if (buffer.getNumChannels() == 1)
    {
        stereoBuffer.setSize (2, numSamples, false, false, true);
        stereoBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
        stereoBuffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        processBuffer = stereoBuffer;
    }

    dsp::AudioBlock<float> block { processBuffer };

    dryWetMixer.setWetMixProportion (*mixParam);
    dryWetMixer.pushDrySamples (block);

    const auto delayTypeIndex = (int) *delayTypeParam;
    if (delayTypeIndex != prevDelayTypeIndex)
    {
        for (auto& delaySet : cleanDelay)
            for (auto& delay : delaySet)
                delay.reset();

        for (auto& delaySet : lofiDelay)
            for (auto& delay : delaySet)
                delay.reset();

        prevDelayTypeIndex = delayTypeIndex;
    }

    if (delayTypeIndex == 0)
        processChorus (processBuffer, cleanDelay);
    else if (delayTypeIndex == 1)
        processChorus (processBuffer, lofiDelay);

    dryWetMixer.mixWetSamples (block);

    outputBuffers.getReference (0) = &processBuffer;
}


void Chorus::addToPopupMenu (PopupMenu& menu)
{
    menu.addSectionHeader ("Delay Type");
    int itemID = 0;

    auto* delayTypeChoiceParam = dynamic_cast<AudioParameterChoice*> (vts.getParameter (delayTypeTag));
    delayTypeAttach = std::make_unique<ParameterAttachment> (
        *delayTypeChoiceParam, [=] (float) {}, vts.undoManager);

    for (const auto [index, delayTypeChoice] : sst::cpputils::enumerate (delayTypeChoiceParam->choices))
    {
        PopupMenu::Item delayTypeItem;
        delayTypeItem.itemID = ++itemID;
        delayTypeItem.text = delayTypeChoice;
        delayTypeItem.action = [&, newParamVal = delayTypeChoiceParam->convertTo0to1 ((float) index)]
        {
            delayTypeAttach->setValueAsCompleteGesture (newParamVal);
        };
        delayTypeItem.colour = (delayTypeChoiceParam->getIndex() == (int) index) ? uiOptions.powerColour : Colours::white;

        menu.addItem (delayTypeItem);
    }

    menu.addSeparator();
}
