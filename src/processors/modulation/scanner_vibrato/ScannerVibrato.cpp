#include "ScannerVibrato.h"
#include "processors/BufferHelpers.h"
#include "processors/ParameterHelpers.h"

namespace
{
const String rateTag = "rate";
const String depthTag = "depth";
const String mixTag = "mix";
const String modeTag = "mode";
const String stereoTag = "stereo";

constexpr auto o16 = 1.0f / 16.0f;
float ramp_up (float x, int off)
{
    const auto y = 16.0f * (x - (float) off * o16);
    return (y > 1.0f || y < 0.0f) ? 0.0f : y;
}
float ramp_down (float x, int off)
{
    const auto y = 1.0f - 16.0f * (x - (float) off * o16);
    return (y > 1.0f || y < 0.0f) ? 0.0f : y;
}
} // namespace

ScannerVibrato::ScannerVibrato (UndoManager* um) : BaseProcessor ("Scanner Vibrato",
                                                                  createParameterLayout(),
                                                                  um,
                                                                  magic_enum::enum_count<InputPort>(),
                                                                  magic_enum::enum_count<OutputPort>())
{
    using namespace ParameterHelpers;
    loadParameterPointer (rateHzParam, vts, rateTag);
    loadParameterPointer (mixParam, vts, mixTag);
    loadParameterPointer (modeParam, vts, modeTag);
    loadParameterPointer (stereoParam, vts, stereoTag);

    addPopupMenuParameter (stereoTag);

    depthParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, depthTag));
    depthParam.setRampLength (0.05);
    depthParam.mappingFunction = [] (float x)
    {
        return 0.5f * x;
    };

    const auto initTable = [this] (int index, auto&& func)
    {
        tapMixTable[index].initialise (func, 0.0f, 1.0f, 1024);
    };
    initTable (0, [] (float x)
               { return ramp_up (x, 0) + ramp_down (x, 1); });
    initTable (1, [] (float x)
               { return ramp_up (x, 1) + ramp_down (x, 2) + ramp_up (x, 15) + ramp_down (x, 0); });
    initTable (2, [] (float x)
               { return ramp_up (x, 2) + ramp_down (x, 3) + ramp_up (x, 14) + ramp_down (x, 15); });
    initTable (3, [] (float x)
               { return ramp_up (x, 3) + ramp_down (x, 4) + ramp_up (x, 13) + ramp_down (x, 14); });
    initTable (4, [] (float x)
               { return ramp_up (x, 4) + ramp_down (x, 5) + ramp_up (x, 12) + ramp_down (x, 13); });
    initTable (5, [] (float x)
               { return ramp_up (x, 5) + ramp_down (x, 6) + ramp_up (x, 11) + ramp_down (x, 12); });
    initTable (6, [] (float x)
               { return ramp_up (x, 6) + ramp_down (x, 7) + ramp_up (x, 10) + ramp_down (x, 11); });
    initTable (7, [] (float x)
               { return ramp_up (x, 7) + ramp_down (x, 8) + ramp_up (x, 9) + ramp_down (x, 10); });
    initTable (8, [] (float x)
               { return ramp_up (x, 8) + ramp_down (x, 9); });

    uiOptions.backgroundColour = Colour { 0xff95756d };
    uiOptions.powerColour = Colour { 0xffe5e3dc };
    uiOptions.info.description = "Virtual analog emulation of the scanner vibrato/chorus effect from the Hammond Organ.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };

    routeExternalModulation ({ ModulationInput }, { ModulationOutput });
    disableWhenInputConnected ({ rateTag }, ModulationInput);
}

ParamLayout ScannerVibrato::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();
    createFreqParameter (params, rateTag, "Rate", 0.5f, 10.0f, 6.0f, 6.0f);
    createPercentParameter (params, depthTag, "Depth", 0.5f);
    createPercentParameter (params, mixTag, "Mix", 0.5f);

    StringArray modeChoices;
    for (const auto& choice : magic_enum::enum_names<ScannerVibratoWDF::Mode>())
        modeChoices.add (choice.data());
    emplace_param<chowdsp::ChoiceParameter> (params, modeTag, "Mode", modeChoices, 0);
    emplace_param<chowdsp::BoolParameter> (params, stereoTag, "Stereo", false);

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
    
    for (int ch = 0; ch < 2; ++ch)
    {
        wdf[ch].prepare ((float) sampleRate);
        tapsOutBuffer[ch].setMaxSize (ScannerVibratoWDF::numTaps, samplesPerBlock);
        modsMixBuffer[ch].setMaxSize (ScannerVibratoWDF::numTaps, samplesPerBlock);
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
        
        // generate mod mix arrays
        auto** modMixDataCh0 = modsMixBuffer[0].getArrayOfWritePointers();
        FloatVectorOperations::add (modMixDataCh0[0], modOutBuffer.getReadPointer (0), 1.0f, numSamples);
        FloatVectorOperations::multiply (modMixDataCh0[0], depthParam.getSmoothedBuffer(), numSamples); // this also multiplies the signal by 0.5
        
        
        auto** modMixDataCh1 = modsMixBuffer[1].getArrayOfWritePointers();
        FloatVectorOperations::add (modMixDataCh1[0], modOutBuffer.getReadPointer (0), 1.0f, numSamples);
        FloatVectorOperations::multiply (modMixDataCh1[0], depthParam.getSmoothedBuffer(), numSamples); // this also multiplies the signal by 0.5
        
        if (! stereoMode)
        {
            for (int i = ScannerVibratoWDF::numTaps - 1; i >= 0; --i)
                tapMixTable[i].process (modMixDataCh0[0], modMixDataCh0[i], numSamples);
        }

        const auto& audioInBuffer = getInputBuffer (AudioInput);
        const auto numInChannels = audioInBuffer.getNumChannels();
        const auto numOutChannels = stereoMode ? 2 : numInChannels;
        audioOutBuffer.setSize (numOutChannels, numSamples, false, false, true);
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

            auto* outData = audioOutBuffer.getWritePointer (ch);
            if(stereoMode)
            {
                if(ch == 1)
                {
                    // invert right phase
                    FloatVectorOperations::negate (modMixDataCh1[0], modMixDataCh1[0], numSamples);
                    FloatVectorOperations::add (modMixDataCh1[0], 1.0f, numSamples);
                        
                    // process right and add to output
                    for (int i = ScannerVibratoWDF::numTaps - 1; i >= 0; --i)
                    {
                        tapMixTable[i].process (modMixDataCh1[0], modMixDataCh1[i], numSamples);
                        FloatVectorOperations::addWithMultiply (outData, tapsOutData[i], modMixDataCh1[i], numSamples);
                    }
                }
                
                else
                {
                    // process left and add to output
                    for (int i = ScannerVibratoWDF::numTaps - 1; i >= 0; --i)
                    {
                        tapMixTable[i].process (modMixDataCh0[0], modMixDataCh0[i], numSamples);
                        FloatVectorOperations::addWithMultiply (outData, tapsOutData[i], modMixDataCh0[i], numSamples);
                    }
                }
            }
            else
            {
                
                for (int i = ScannerVibratoWDF::numTaps - 1; i >= 0; --i)
                    FloatVectorOperations::addWithMultiply (outData, tapsOutData[i], modMixDataCh0[i], numSamples);
            }
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
