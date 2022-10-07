#include "Flanger.h"
#include "../BufferHelpers.h"
#include "../ParameterHelpers.h"

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

Flanger::Flanger (UndoManager* um) : BaseProcessor ("Flanger",
                                                    createParameterLayout(),
                                                    um,
                                                    magic_enum::enum_count<InputPort>(),
                                                    magic_enum::enum_count<OutputPort>())
{
    //    chowdsp::FloatParameter* rateParam = nullptr;
    //    chowdsp::FloatParameter* fbParam = nullptr;
    //    chowdsp::FloatParameter* delayAmountParam = nullptr;
    //    chowdsp::FloatParameter* delayOffsetParam = nullptr;
    //    dsp::DryWetMixer<float> dryWetMixer;

    using namespace ParameterHelpers;
    loadParameterPointer (rateParam, vts, "rate");
    loadParameterPointer (delayAmountParam, vts, "delay amount");
    loadParameterPointer (delayOffsetParam, vts, "delay amount");
    loadParameterPointer (fbParam, vts, "feedback");
    loadParameterPointer (mixParam, vts, "mix");
    delayTypeParam = vts.getRawParameterValue (delayTypeTag);

    addPopupMenuParameter (delayTypeTag);

    uiOptions.backgroundColour = Colour (48, 25, 188);
    uiOptions.powerColour = Colours::yellow.brighter (0.1f);
    uiOptions.info.description = "A multi-phase chorus effect. Use the right-click menu to enable lo-fi mode.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ "rate" }, ModulationInput);
}

ParamLayout Flanger::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    //    createPercentParameter (params, "rate", "Rate", 0.5f);
    //    createPercentParameter (params, "depth", "Depth", 0.5f);
    //    createPercentParameter (params, "feedback", "Feedback", 0.0f);
    //    createPercentParameter (params, "mix", "Mix", 0.5f);
    //
    //    emplace_param<AudioParameterChoice> (params, delayTypeTag, "Delay Type", StringArray { "Clean", "Lo-Fi" }, 0);

    return { params.begin(), params.end() };
}

void Flanger::prepare (double sampleRate, int samplesPerBlock)
{
    //    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    //    dsp::ProcessSpec monoSpec { sampleRate, (uint32) samplesPerBlock, 1 };
    //    fs = (float) sampleRate;
    //
    //    for (int ch = 0; ch < 2; ++ch)
    //    {
    //        for (int i = 0; i < delaysPerChannel; ++i)
    //        {
    //            cleanDelay[ch][i].prepare (monoSpec);
    //            lofiDelay[ch][i].prepare (monoSpec);
    //
    //            slowLFOs[ch][i].prepare (monoSpec);
    //            fastLFOs[ch][i].prepare (monoSpec);
    //
    //            slowLFOData[ch][i].resize ((size_t) samplesPerBlock, 0.0f);
    //            fastLFOData[ch][i].resize ((size_t) samplesPerBlock, 0.0f);
    //        }
    //
    //        slowSmooth[ch].reset (sampleRate, 0.05);
    //        fastSmooth[ch].reset (sampleRate, 0.05);
    //
    //        feedbackState[ch] = 0.0f;
    //        fbSmooth[ch].reset (sampleRate, 0.01);
    //}

    //    // set phase offsets
    //    constexpr float piOver3 = MathConstants<float>::pi / 3.0f;
    //    slowLFOs[0][0].reset (-piOver3);
    //    fastLFOs[0][0].reset (-piOver3);
    //    slowLFOs[1][1].reset (piOver3);
    //    fastLFOs[1][1].reset (piOver3);
    //
    //    aaFilter.prepare (spec);
    //    aaFilter.setCutoffFrequency (12000.0f);
    //
    //    dryWetMixer.prepare (spec);
    //    dryWetMixer.setMixingRule (dsp::DryWetMixingRule::sin3dB);
    //
    //    dcBlocker.prepare (spec);
    //    dcBlocker.setCutoffFrequency (60.0f);
    //
    //    audioOutBuffer.setSize (2, samplesPerBlock);
    //    modOutBuffer.setSize (1, samplesPerBlock);
    //
    //    for (auto& filt : hilbertFilter)
    //        filt.reset();
    //
    //    bypassNeedsReset = false;
}

//void Flanger::processModulation (int numSamples)
//{
//    modOutBuffer.setSize (1, numSamples, false, false, true);
//    if (inputsConnected.contains (ModulationInput))
//    {
//        // get modulation buffer from input (-1, 1)
//        const auto& modInputBuffer = getInputBuffer (ModulationInput);
//        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
//
//        auto phaseShiftSlowLFO = [this, numSamples, filterIndex = 0] (float* lfoIn, float* lfoOut) mutable
//        {
//            // trying ot use more of the Hilbert filtrers than we have available!
//            jassert (filterIndex < (int) std::size (hilbertFilter));
//
//            auto& filter = hilbertFilter[filterIndex++];
//            for (int n = 0; n < numSamples; ++n)
//                std::tie (lfoIn[n], lfoOut[n]) = filter.process (lfoIn[n]);
//        };
//
//        auto makeFastLFO = [numSamples] (const float* slowLFO, float* fastLFO)
//        {
//            FloatVectorOperations::abs (fastLFO, slowLFO, numSamples);
//            FloatVectorOperations::add (fastLFO, -0.5f, numSamples);
//            FloatVectorOperations::abs (fastLFO, fastLFO, numSamples);
//            FloatVectorOperations::add (fastLFO, -0.25f, numSamples);
//            FloatVectorOperations::multiply (fastLFO, 4.0f, numSamples);
//        };
//
//        auto* modOutData = modOutBuffer.getReadPointer (0);
//
//        FloatVectorOperations::copy (slowLFOData[0][0].data(), modOutData, numSamples);
//        phaseShiftSlowLFO (slowLFOData[0][0].data(), slowLFOData[0][1].data());
//        FloatVectorOperations::copy (slowLFOData[1][0].data(), slowLFOData[0][1].data(), numSamples);
//        phaseShiftSlowLFO (slowLFOData[1][0].data(), slowLFOData[1][1].data());
//
//        makeFastLFO (slowLFOData[0][0].data(), fastLFOData[0][0].data());
//        makeFastLFO (slowLFOData[0][1].data(), fastLFOData[0][1].data());
//        makeFastLFO (slowLFOData[1][0].data(), fastLFOData[1][0].data());
//        makeFastLFO (slowLFOData[1][1].data(), fastLFOData[1][1].data());
//    }
//    else
//    {
//        auto slowRate = rate1Low * std::pow (rate1High / rate1Low, *rateParam);
//        auto fastRate = rate2Low * std::pow (rate2High / rate2Low, *rateParam);
//
//        for (int ch = 0; ch < 2; ++ch)
//        {
//            for (int i = 0; i < delaysPerChannel; ++i)
//            {
//                slowLFOs[ch][i].setFrequency (slowRate);
//                fastLFOs[ch][i].setFrequency (fastRate);
//            }
//
//            for (int i = 0; i < delaysPerChannel; ++i)
//            {
//                auto* slowData = slowLFOData[ch][i].data();
//                auto* fastData = fastLFOData[ch][i].data();
//
//                for (int n = 0; n < numSamples; ++n)
//                {
//                    slowData[n] = slowLFOs[ch][i].processSample();
//                    fastData[n] = fastLFOs[ch][i].processSample();
//                }
//            }
//        }
//
//        auto* modOutData = modOutBuffer.getWritePointer (0);
//        FloatVectorOperations::copy (modOutData, slowLFOData[0][0].data(), numSamples);
//        FloatVectorOperations::add (modOutData, fastLFOData[0][0].data(), numSamples);
//        FloatVectorOperations::multiply (modOutData, 0.5f, numSamples); // make sure the end result never goes larger than 1!
//    }
//}

//template <typename DelayArrType>
//void Flanger::processChorus (AudioBuffer<float>& buffer, DelayArrType& delay)
//{
//    jassert (buffer.getNumChannels() == 2); // always processing in stereo
//    const auto numSamples = buffer.getNumSamples();
//
//    for (int ch = 0; ch < 2; ++ch)
//    {
//        for (int i = 0; i < delaysPerChannel; ++i)
//            delay[ch][i].setFilterFreq (10000.0f);
//
//        auto fbAmount = std::sqrt (fbParam->getCurrentValue());
//        if constexpr (std::is_same_v<DelayArrType, decltype (lofiDelay)>)
//            fbAmount *= 0.4f;
//        else
//            fbAmount *= 0.5f;
//
//        fbSmooth[ch].setTargetValue (fbAmount);
//        slowSmooth[ch].setTargetValue (delay1Ms * 0.001f * fs * *depthParam);
//        fastSmooth[ch].setTargetValue (delay2Ms * 0.001f * fs * *depthParam);
//
//        auto* x = buffer.getWritePointer (ch);
//        for (int n = 0; n < numSamples; ++n)
//        {
//            auto xIn = std::tanh (x[n] * 0.75f - feedbackState[ch]);
//
//            auto slowSmoothMult = slowSmooth[ch].getNextValue();
//            auto fastSmoothMult = fastSmooth[ch].getNextValue();
//
//            x[n] = 0.0f;
//            for (int i = 0; i < delaysPerChannel; ++i)
//            {
//                float delayAmt = slowSmoothMult * (1.0f + 0.95f * slowLFOData[ch][i][(size_t) n]);
//                delayAmt += fastSmoothMult * (1.0f + 0.95f * fastLFOData[ch][i][(size_t) n]);
//
//                delay[ch][i].setDelay (delayAmt);
//                delay[ch][i].pushSample (0, xIn);
//                x[n] += delay[ch][i].popSample (0);
//            }
//
//            x[n] = aaFilter.processSample (ch, x[n]);
//
//            feedbackState[ch] = fbSmooth[ch].getNextValue() * x[n];
//            feedbackState[ch] = dcBlocker.processSample (ch, feedbackState[ch]);
//        }
//    }
//}

void Flanger::processAudio (AudioBuffer<float>& buffer)
{
}

void Flanger::processAudioBypassed (AudioBuffer<float>& buffer)
{
}
//    const auto numSamples = buffer.getNumSamples();
//    processModulation (numSamples);
//
//    if (inputsConnected.contains (AudioInput))
//    {
//        const auto& audioInBuffer = getInputBuffer (AudioInput);
//        const auto numInChannels = audioInBuffer.getNumChannels();
//
//        // always have a stereo output!
//        audioOutBuffer.setSize (2, numSamples, false, false, true);
//        audioOutBuffer.copyFrom (0, 0, audioInBuffer, 0, 0, numSamples);
//        audioOutBuffer.copyFrom (1, 0, audioInBuffer, 1 % numInChannels, 0, numSamples);
//
//        dsp::AudioBlock<float> block { audioOutBuffer };
//
//        dryWetMixer.setWetMixProportion (*mixParam);
//        dryWetMixer.pushDrySamples (block);
//
//        const auto delayTypeIndex = (int) *delayTypeParam;
//        if (delayTypeIndex != prevDelayTypeIndex)
//        {
//            for (auto& delaySet : cleanDelay)
//                for (auto& delay : delaySet)
//                    delay.reset();
//
//            for (auto& delaySet : lofiDelay)
//                for (auto& delay : delaySet)
//                    delay.reset();
//
//            prevDelayTypeIndex = delayTypeIndex;
//        }
//
//        if (delayTypeIndex == 0)
//            processChorus (audioOutBuffer, cleanDelay);
//        else if (delayTypeIndex == 1)
//            processChorus (audioOutBuffer, lofiDelay);
//
//        dryWetMixer.mixWetSamples (block);
//    }
//    else
//    {
//        audioOutBuffer.setSize (1, numSamples, false, false, true);
//        audioOutBuffer.clear();
//    }
//
//    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
//    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
//
//    bypassNeedsReset = true;
//}

//void Chorus::processAudioBypassed (AudioBuffer<float>& buffer)
//{
//    if (bypassNeedsReset)
//    {
//        for (auto& delaySet : cleanDelay)
//            for (auto& delay : delaySet)
//                delay.reset();
//
//        for (auto& delaySet : lofiDelay)
//            for (auto& delay : delaySet)
//                delay.reset();
//
//        bypassNeedsReset = false;
//    }
//
//    const auto numSamples = buffer.getNumSamples();
//
//    modOutBuffer.setSize (1, numSamples, false, false, true);
//    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
//    {
//        // get modulation buffer from input (-1, 1)
//        const auto& modInputBuffer = getInputBuffer (ModulationInput);
//        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
//    }
//    else
//    {
//        modOutBuffer.clear();
//    }
//
//    if (inputsConnected.contains (AudioInput))
//    {
//        const auto& audioInBuffer = getInputBuffer (AudioInput);
//        const auto numChannels = audioInBuffer.getNumChannels();
//        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
//        audioOutBuffer.makeCopyOf (audioInBuffer, true);
//    }
//    else
//    {
//        audioOutBuffer.setSize (1, numSamples, false, false, true);
//        audioOutBuffer.clear();
//    }
//
//    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
//    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
//}
