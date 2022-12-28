#include "MuffClipper.h"
#include "../../ParameterHelpers.h"

namespace
{
const auto cutoffRange = ParameterHelpers::createNormalisableRange (500.0f, 22000.0f, 1200.0f);
float harmParamToCutoffHz (float harmParam)
{
    return cutoffRange.convertFrom0to1 (harmParam);
}

const auto sustainRange = ParameterHelpers::createNormalisableRange (0.4f, 2.0f, 1.0f);
const auto levelRange = ParameterHelpers::createNormalisableRange (-60.0f, 0.0f, -9.0f);
} // namespace

MuffClipper::MuffClipper (UndoManager* um) : BaseProcessor ("Muff Clipper", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (sustainParam, vts, "sustain");
    loadParameterPointer (harmParam, vts, "harmonics");
    loadParameterPointer (levelParam, vts, "level");
    clip1Param.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, "clip1"));
    clip2Param.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, "clip2"));
    smoothingParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, "smoothing"));
    hiQParam = vts.getRawParameterValue ("high_q");

    addPopupMenuParameter ("high_q");

    uiOptions.backgroundColour = Colours::darkgrey.brighter (0.3f).withRotatedHue (0.2f);
    uiOptions.powerColour = Colours::red.brighter (0.15f);
    uiOptions.info.description = "Fuzz effect based on the drive stage from the Electro-Harmonix Big Muff Pi.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout MuffClipper::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();

    createPercentParameter (params, "sustain", "Gain", 0.5f);
    createPercentParameter (params, "harmonics", "Harm.", 0.65f);
    createBipolarPercentParameter (params, "smoothing", "Smooth", 0.0f);
    createBipolarPercentParameter (params, "clip1", "+Clip", 0.0f);
    createBipolarPercentParameter (params, "clip2", "-Clip", 0.0f);
    createPercentParameter (params, "level", "Level", 0.65f);

    emplace_param<AudioParameterBool> (params, "high_q", "High Quality", true);

    return { params.begin(), params.end() };
}

void MuffClipper::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;

    cutoffSmooth.reset (sampleRate, 0.02);
    cutoffSmooth.setCurrentAndTargetValue (harmParamToCutoffHz (*harmParam));
    for (auto& filt : inputFilter)
    {
        filt.calcCoefs (cutoffSmooth.getTargetValue(), fs);
        filt.reset();
    }

    clip1Param.setRampLength (0.05);
    clip1Param.mappingFunction = [fs = this->fs] (float val)
    {
        return val + 1.0;
    };
    clip1Param.prepare (sampleRate, samplesPerBlock);

    clip2Param.setRampLength (0.05);
    clip2Param.mappingFunction = [fs = this->fs] (float val)
    {
        return val + 1.0;
    };
    clip2Param.prepare (sampleRate, samplesPerBlock);

    smoothingParam.setRampLength (0.05);
    smoothingParam.mappingFunction = [fs = this->fs] (float val)
    {
        return MuffClipperStage::getGC12 (fs, val);
    };
    smoothingParam.prepare (sampleRate, samplesPerBlock);

    stage.prepare (sampleRate);

    auto spec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 2 };

    sustainGain.prepare (spec);
    sustainGain.setRampDurationSeconds (0.02);

    outLevel.prepare (spec);
    outLevel.setRampDurationSeconds (0.02);

    for (auto& filt : dcBlocker)
    {
        filt.calcCoefs (16.0f, fs);
        filt.reset();
    }

    maxBlockSize = samplesPerBlock;
    doPrebuffering();
}

void MuffClipper::doPrebuffering()
{
    AudioBuffer<float> buffer (2, maxBlockSize);
    for (int i = 0; i < 10000; i += maxBlockSize)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void MuffClipper::processInputStage (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    cutoffSmooth.setTargetValue (harmParamToCutoffHz (*harmParam));
    if (cutoffSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            auto* x = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                inputFilter[0].calcCoefs (cutoffSmooth.getNextValue(), fs);
                x[n] = inputFilter[0].processSample (x[n]);
            }
        }
        else if (numChannels == 2)
        {
            auto* xL = buffer.getWritePointer (0);
            auto* xR = buffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                auto cutoffHz = cutoffSmooth.getNextValue();

                inputFilter[0].calcCoefs (cutoffHz, fs);
                xL[n] = inputFilter[0].processSample (xL[n]);

                inputFilter[1].calcCoefs (cutoffHz, fs);
                xR[n] = inputFilter[1].processSample (xR[n]);
            }
        }
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            inputFilter[ch].calcCoefs (cutoffSmooth.getNextValue(), fs);
            inputFilter[ch].processBlock (buffer.getWritePointer (ch), numSamples);
        }
    }

    sustainGain.setGainLinear (sustainRange.convertFrom0to1 (*sustainParam));
    dsp::AudioBlock<float> block { buffer };
    sustainGain.process (dsp::ProcessContextReplacing<float> { block });
}

void MuffClipper::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    processInputStage (buffer);

    clip1Param.process (numSamples);
    clip2Param.process (numSamples);
    smoothingParam.process (numSamples);
    const auto useHighQualityMode = hiQParam->load() == 1.0f;
    if (useHighQualityMode)
    {
        stage.processBlock<true> (buffer, clip1Param, clip2Param, smoothingParam);
    }
    else
    {
        stage.processBlock<false> (buffer, clip1Param, clip2Param, smoothingParam);
    }

    for (int ch = 0; ch < numChannels; ++ch)
        dcBlocker[ch].processBlock (buffer.getWritePointer (ch), numSamples);

    auto outGain = Decibels::decibelsToGain (levelRange.convertFrom0to1 (*levelParam), levelRange.start);
    outGain *= Decibels::decibelsToGain (13.0f); // makeup from level lost in clipping stage
    outGain *= -1.0f;
    outLevel.setGainLinear (outGain);
    dsp::AudioBlock<float> block { buffer };
    outLevel.process (dsp::ProcessContextReplacing<float> { block });
}
