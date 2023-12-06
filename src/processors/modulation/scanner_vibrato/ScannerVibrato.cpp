#include "ScannerVibrato.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"

namespace ScannerVibratoTags
{
const String rateTag = "rate";
const String depthTag = "depth";
const String mixTag = "mix";
const String modeTag = "mode";
const String stereoTag = "stereo";
} // namespace

ScannerVibrato::ScannerVibrato (UndoManager* um) : BaseProcessor (
    "Scanner Vibrato",
    createParameterLayout(),
    InputPort {},
    OutputPort {},
    um,
    [] (InputPort port)
    {
        if (port == InputPort::ModulationInput)
            return PortType::modulation;
        return PortType::audio;
    },
    [] (OutputPort port)
    {
        if (port == OutputPort::ModulationOutput)
            return PortType::modulation;
        return PortType::audio;
    })
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateHzParam, vts, ScannerVibratoTags::rateTag);
    loadParameterPointer (mixParam, vts, ScannerVibratoTags::mixTag);
    loadParameterPointer (modeParam, vts, ScannerVibratoTags::modeTag);
    loadParameterPointer (stereoParam, vts, ScannerVibratoTags::stereoTag);

    addPopupMenuParameter (ScannerVibratoTags::stereoTag);

    depthParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, ScannerVibratoTags::depthTag));
    depthParam.setRampLength (0.05);
    depthParam.mappingFunction = [] (float x)
    {
        return 0.5f * x;
    };

    static constexpr auto o16 = 1.0f / 16.0f;
    const auto ramp_up = [] (float x, int off)
    {
        const auto y = 16.0f * (x - (float) off * o16);
        return (y > 1.0f || y < 0.0f) ? 0.0f : y;
    };
    const auto ramp_down = [] (float x, int off)
    {
        const auto y = 1.0f - 16.0f * (x - (float) off * o16);
        return (y > 1.0f || y < 0.0f) ? 0.0f : y;
    };
    const auto initTable = [this] (int index, auto&& func)
    {
        tapMixTable[index].initialise (func, 0.0f, 1.0f, 1024);
    };
    initTable (0, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 0) + ramp_down (x, 1); });
    initTable (1, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 1) + ramp_down (x, 2) + ramp_up (x, 15) + ramp_down (x, 0); });
    initTable (2, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 2) + ramp_down (x, 3) + ramp_up (x, 14) + ramp_down (x, 15); });
    initTable (3, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 3) + ramp_down (x, 4) + ramp_up (x, 13) + ramp_down (x, 14); });
    initTable (4, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 4) + ramp_down (x, 5) + ramp_up (x, 12) + ramp_down (x, 13); });
    initTable (5, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 5) + ramp_down (x, 6) + ramp_up (x, 11) + ramp_down (x, 12); });
    initTable (6, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 6) + ramp_down (x, 7) + ramp_up (x, 10) + ramp_down (x, 11); });
    initTable (7, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 7) + ramp_down (x, 8) + ramp_up (x, 9) + ramp_down (x, 10); });
    initTable (8, [ramp_up, ramp_down] (float x)
               { return ramp_up (x, 8) + ramp_down (x, 9); });

    uiOptions.backgroundColour = Colour { 0xff95756d };
    uiOptions.powerColour = Colour { 0xffe5e3dc };
    uiOptions.info.description = "Virtual analog emulation of the scanner vibrato/chorus effect from the Hammond Organ.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    disableWhenInputConnected ({ ScannerVibratoTags::rateTag }, ModulationInput);
}

ParamLayout ScannerVibrato::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, ScannerVibratoTags::rateTag, "Rate", 0.5f, 10.0f, 6.0f, 6.0f);
    createPercentParameter (params, ScannerVibratoTags::depthTag, "Depth", 0.5f);
    createPercentParameter (params, ScannerVibratoTags::mixTag, "Mix", 0.5f);

    StringArray modeChoices;
    for (const auto& choice : magic_enum::enum_names<ScannerVibratoWDF::Mode>())
        modeChoices.add (choice.data());
    emplace_param<chowdsp::ChoiceParameter> (params, ScannerVibratoTags::modeTag, "Mode", modeChoices, 0);
    emplace_param<chowdsp::BoolParameter> (params, ScannerVibratoTags::stereoTag, "Stereo", false);

    return { params.begin(), params.end() };
}

void ScannerVibrato::prepare (double sampleRate, int samplesPerBlock)
{
    depthParam.prepare (sampleRate, samplesPerBlock);

    const auto spec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 2 };
    const auto monoSpec = dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, 1 };

    modSource.prepare (monoSpec);
    mixer.prepare (spec);
    mixer.setMixingRule (dsp::DryWetMixingRule::sin3dB);

    mod01Buffer.setMaxSize (1, samplesPerBlock);
    modsMixBuffer.setMaxSize (ScannerVibratoWDF::numTaps, samplesPerBlock);
    for (int ch = 0; ch < 2; ++ch)
    {
        wdf[ch].prepare ((float) sampleRate);
        tapsOutBuffer[ch].setMaxSize (ScannerVibratoWDF::numTaps, samplesPerBlock);
    }

    modOutBuffer.setSize (1, samplesPerBlock);
    audioOutBuffer.setSize (2, samplesPerBlock);
}

void ScannerVibrato::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    depthParam.process (numSamples);

    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else // create our own modulation signal
    {
        modOutBuffer.clear();
        modSource.setFrequency (*rateHzParam);
        modSource.processBlock (modOutBuffer);
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto stereoMode = stereoParam->get();

        // get modulation data in [0, 1] range
        auto* modData01 = mod01Buffer.getWritePointer (0);
        FloatVectorOperations::multiply (modData01, modOutBuffer.getReadPointer (0), depthParam.getSmoothedBuffer(), numSamples); // this also multiplies the signal by 0.5
        FloatVectorOperations::add (modData01, 0.5f, numSamples);

        // generate mod mix arrays
        auto** modMixData = modsMixBuffer.getArrayOfWritePointers();
        for (int i = 0; i < ScannerVibratoWDF::numTaps; ++i)
            tapMixTable[i].process (modData01, modMixData[i], numSamples);

        // handle input num channels
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numInChannels = audioInBuffer.getNumChannels();
        const auto numOutChannels = stereoMode ? 2 : numInChannels;
        audioOutBuffer.setSize (numOutChannels, numSamples, false, false, true);

        // push dry signal into mixer
        mixer.setWetMixProportion (*mixParam);
        if (stereoMode && numInChannels == 1)
        {
            for (int ch = 0; ch < numOutChannels; ++ch)
                audioOutBuffer.copyFrom (ch, 0, audioInBuffer, ch % numInChannels, 0, numSamples);

            mixer.pushDrySamples (audioOutBuffer);
        }
        else
        {
            mixer.pushDrySamples (audioInBuffer);
        }

        audioOutBuffer.clear();
        const auto modeIndex = modeParam->getIndex();
        using Mode = ScannerVibratoWDF::Mode;
        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            // process tap outs
            tapsOutBuffer[ch].setCurrentSize (ScannerVibratoWDF::numTaps, numSamples);
            auto** tapsOutData = tapsOutBuffer[ch].getArrayOfWritePointers();
            magic_enum::enum_switch (
                [this,
                 ch,
                 numSamples,
                 tapsOutData,
                 inData = audioInBuffer.getReadPointer (ch % numInChannels)] (auto mode)
                {
                    constexpr Mode mode_c = mode;
                    wdf[ch].processBlock<mode_c> (inData, tapsOutData, numSamples);
                },
                static_cast<Mode> (modeIndex));

            // process with modulation
            auto* outData = audioOutBuffer.getWritePointer (ch);
            if (stereoMode && ch == 1)
            {
                // invert right phase
                FloatVectorOperations::negate (modData01, modData01, numSamples);
                FloatVectorOperations::add (modData01, 1.0f, numSamples);

                // recompute mod mix data
                for (int i = 0; i < ScannerVibratoWDF::numTaps; ++i)
                    tapMixTable[i].process (modData01, modMixData[i], numSamples);
            }

            for (int i = ScannerVibratoWDF::numTaps - 1; i >= 0; --i)
                FloatVectorOperations::addWithMultiply (outData, tapsOutData[i], modMixData[i], numSamples);
        }

        audioOutBuffer.applyGain (-Decibels::decibelsToGain (3.0f));
        mixer.mixWetSamples (audioOutBuffer);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}

void ScannerVibrato::processAudioBypassed (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    modOutBuffer.setSize (1, numSamples, false, false, true);
    if (inputsConnected.contains (ModulationInput)) // make mono and pass samples through
    {
        // get modulation buffer from input (-1, 1)
        const auto& modInputBuffer = getInputBuffer (ModulationInput);
        BufferHelpers::collapseToMonoBuffer (modInputBuffer, modOutBuffer);
    }
    else
    {
        modOutBuffer.clear();
    }

    if (inputsConnected.contains (AudioInput))
    {
        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numChannels = audioInBuffer.getNumChannels();
        audioOutBuffer.setSize (numChannels, numSamples, false, false, true);
        audioOutBuffer.makeCopyOf (audioInBuffer, true);
    }
    else
    {
        audioOutBuffer.setSize (1, numSamples, false, false, true);
        audioOutBuffer.clear();
    }

    outputBuffers.getReference (AudioOutput) = &audioOutBuffer;
    outputBuffers.getReference (ModulationOutput) = &modOutBuffer;
}
