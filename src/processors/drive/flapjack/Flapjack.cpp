#include "Flapjack.h"
#include "processors/ParameterHelpers.h"

//https://aionfx.com/news/tracing-journal-crowther-hot-cake/
// Mode 1:
// - transistor is effectively bypassed
// - bias voltage = 4.9V
// Mode 2:
// - transistor is _active_
// - bias voltage = 4V

namespace
{
const auto driveTag = "drive";
const auto presenceTag = "presence";
const auto levelTag = "level";
const auto modeTag = "mode";
const auto lowCutTag = "lowcut";
} // namespace

Flapjack::Flapjack (UndoManager* um)
    : BaseProcessor ("Flapjack", createParameterLayout(), um)
{
    using namespace ParameterHelpers;

    driveParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, driveTag));
    driveParam.setRampLength (0.025);
    driveParam.mappingFunction = [] (float x)
    {
        return chowdsp::Power::ipow<2> (1.0f - x);
    };
    presenceParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, presenceTag));
    presenceParam.setRampLength (0.025);
    lowCutParam.setParameterHandle (getParameterPointer<chowdsp::FloatParameter*> (vts, lowCutTag));
    lowCutParam.setRampLength (0.025);

    loadParameterPointer (levelParam, vts, levelTag);
    loadParameterPointer (modeParam, vts, modeTag);

    uiOptions.backgroundColour = Colours::whitesmoke.darker (0.1f);
    uiOptions.powerColour = Colours::red.darker (0.2f);
    uiOptions.info.description = "Overdrive effect based on the \"Hot Cake\" overdrive pedal.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout Flapjack::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createPercentParameter (params, driveTag, "Drive", 0.5f);
    createPercentParameter (params, presenceTag, "Presence", 0.5f);
    createFreqParameter (params, lowCutTag, "Low Cut", 20.0f, 750.0f, 100.0f, 100.0f);
    createPercentParameter (params, levelTag, "Level", 0.5f);
    emplace_param<chowdsp::ChoiceParameter> (params, modeTag, "Mode", StringArray { "Op-Amp Clip", "Bluesberry", "Peachy" }, 0);

    return { params.begin(), params.end() };
}

void Flapjack::prepare (double sampleRate, int samplesPerBlock)
{
    driveParam.prepare (sampleRate, samplesPerBlock);
    presenceParam.prepare (sampleRate, samplesPerBlock);
    lowCutParam.prepare (sampleRate, samplesPerBlock);

    for (auto& model : wdf)
        model.prepare (sampleRate);

    dcBlocker.prepare (2);
    dcBlocker.calcCoefs (20.0f, (float) sampleRate);

    level.prepare ({ sampleRate, (uint32_t) samplesPerBlock, 2 });

    // pre-buffering
    AudioBuffer<float> buffer (2, samplesPerBlock);
    for (int i = 0; i < 2000; i += samplesPerBlock)
    {
        buffer.clear();
        processAudio (buffer);
    }
}

void Flapjack::processAudio (AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    driveParam.process (numSamples);
    presenceParam.process (numSamples);
    lowCutParam.process (numSamples);

    const auto clipMode = magic_enum::enum_value<FlapjackClipMode> (modeParam->getIndex());
    magic_enum::enum_switch (
        [this, &buffer] (auto modeVal)
        {
            static constexpr FlapjackClipMode mode = modeVal;
            for (auto [channelIndex, channelData] : chowdsp::buffer_iters::channels (buffer))
            {
                if (driveParam.isSmoothing() || presenceParam.isSmoothing() || lowCutParam.isSmoothing())
                {
                    const auto* driveSmooth = driveParam.getSmoothedBuffer();
                    const auto* presenceSmooth = presenceParam.getSmoothedBuffer();
                    const auto lowCutSmooth = lowCutParam.getSmoothedBuffer();
                    for (auto [n, x] : chowdsp::enumerate (channelData))
                    {
                        wdf[channelIndex].setParams (driveSmooth[n], presenceSmooth[n], lowCutSmooth[n]);
                        x = wdf[channelIndex].template processSample<mode> (x);
                    }
                }
                else
                {
                    wdf[channelIndex].setParams (driveParam.getCurrentValue(), presenceParam.getCurrentValue(), lowCutParam.getCurrentValue());
                    for (auto& x : channelData)
                        x = wdf[channelIndex].template processSample<mode> (x);
                }
            }
        },
        clipMode);

    const auto makeupGain = Decibels::decibelsToGain (-12.0f);
    level.setGainLinear (chowdsp::Power::ipow<2> (levelParam->getCurrentValue()) * -makeupGain);
    level.process (buffer);
    dcBlocker.processBlock (buffer);
}
