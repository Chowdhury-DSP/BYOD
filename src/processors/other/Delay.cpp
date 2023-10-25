#include "Delay.h"
#include "../ParameterHelpers.h"
#include "gui/utils/ModulatableSlider.h"

namespace
{
const String delayTypeTag = "delay_type";
const String pingPongTag = "ping_pong";
const String freqTag = "freq";
const String feedBackTag = "feedback";
const String mixTag = "mix";

const String delayTimeMsTag = "time_ms";
const String tempoSyncTag = "tempo_sync";
const String tempoSyncAmountTag = "delay_time_type";
} // namespace

DelayModule::DelayModule (UndoManager* um) : BaseProcessor ("Delay", createParameterLayout(), um)
{
    using namespace ParameterHelpers;
    loadParameterPointer (freqParam, vts, freqTag);
    loadParameterPointer (feedbackParam, vts, feedBackTag);
    loadParameterPointer (mixParam, vts, mixTag);
    loadParameterPointer (delayTimeMsParam, vts, delayTimeMsTag);
    delayTimeTempoSyncParam = vts.getRawParameterValue (tempoSyncAmountTag);
    tempoSyncOnOffParam = vts.getRawParameterValue (tempoSyncTag);
    delayTypeParam = vts.getRawParameterValue (delayTypeTag);
    pingPongParam = vts.getRawParameterValue (pingPongTag);

    addPopupMenuParameter (delayTypeTag);
    addPopupMenuParameter (pingPongTag);
    addPopupMenuParameter (tempoSyncTag);

    uiOptions.backgroundColour = Colours::cyan.darker (0.1f);
    uiOptions.powerColour = Colours::gold;
    uiOptions.info.description = "A simple delay effect with feedback. Use the right-click menu to enable lo-fi mode or a ping-pong effect.";
    uiOptions.info.authors = StringArray { "Jatin Chowdhury" };
}

ParamLayout DelayModule::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    createTimeMsParameter (params, delayTimeMsTag, "Delay Time", createNormalisableRange (20.0f, 2000.0f, 200.0f), 100.0f);
    createFreqParameter (params, freqTag, "Cutoff", 500.0f, 10000.0f, 4000.0f, 10000.0f);
    createPercentParameter (params, feedBackTag, "Feedback", 0.0f);
    createPercentParameter (params, mixTag, "Mix", 0.5f);

    emplace_param<AudioParameterChoice> (params,
                                         tempoSyncAmountTag,
                                         "Tempo Sync",
                                         StringArray { "1/2", "1/4", "1/8" , "1/8 dotted"},
                                         0);
    emplace_param<AudioParameterBool> (params, tempoSyncTag, "Tempo Sync Delay", false);
    emplace_param<AudioParameterChoice> (params, delayTypeTag, "Delay Type", StringArray { "Clean", "Lo-Fi" }, 0);
    emplace_param<AudioParameterBool> (params, pingPongTag, "Ping-Pong", false);

    return { params.begin(), params.end() };
}

void DelayModule::prepare (double sampleRate, int samplesPerBlock)
{
    fs = (float) sampleRate;
    stereoBuffer.setSize (2, samplesPerBlock);

    dsp::ProcessSpec stereoSpec { sampleRate, (uint32) samplesPerBlock, 2 };
    dsp::ProcessSpec monoSpec = stereoSpec;
    monoSpec.numChannels = 1;

    cleanDelayLine.prepare (stereoSpec);
    lofiDelayLine.prepare (stereoSpec);

    dryWetMixer.prepare (stereoSpec);
    dryWetMixer.setMixingRule (dsp::DryWetMixingRule::balanced);
    dryWetMixerMono.prepare (monoSpec);
    dryWetMixerMono.setMixingRule (dsp::DryWetMixingRule::balanced);

    delaySmooth.reset (sampleRate, 0.1);
    freqSmooth.reset (sampleRate, 0.1);

    feedbackSmoothBuffer.prepare (sampleRate, samplesPerBlock);
    feedbackSmoothBuffer.setRampLength (0.01);

    bypassNeedsReset = false;
}

void DelayModule::releaseMemory()
{
    cleanDelayLine.delay.free();
    lofiDelayLine.free();
}

template <typename DelayType>
void DelayModule::processMonoStereoDelay (AudioBuffer<float>& buffer, DelayType& delayLine)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    dsp::AudioBlock<float> block { buffer };

    auto& dryWet = numChannels == 1 ? dryWetMixerMono : dryWetMixer;
    dryWet.setWetMixProportion (*mixParam * 0.5f);
    dryWet.pushDrySamples (block);

    const auto* fbData = feedbackSmoothBuffer.getSmoothedBuffer();
    if (delaySmooth.isSmoothing() || freqSmooth.isSmoothing())
    {
        if (numChannels == 1)
        {
            auto* x = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                delayLine.setDelay (delaySmooth.getNextValue());
                delayLine.setFilterFreq (freqSmooth.getNextValue());

                auto y = delayLine.popSample (0);
                delayLine.pushSample (0, x[n] + y * fbData[n]);
                x[n] = y;
            }
        }
        else
        {
            auto* xL = buffer.getWritePointer (0);
            auto* xR = buffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                delayLine.setDelay (delaySmooth.getNextValue());
                delayLine.setFilterFreq (freqSmooth.getNextValue());

                auto y = delayLine.popSample (0);
                delayLine.pushSample (0, xL[n] + y * fbData[n]);
                xL[n] = y;

                y = delayLine.popSample (1);
                delayLine.pushSample (1, xR[n] + y * fbData[n]);
                xR[n] = y;
            }
        }
    }
    else
    {
        delayLine.setDelay (delaySmooth.getTargetValue());
        delayLine.setFilterFreq (freqSmooth.getTargetValue());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
            {
                auto y = delayLine.popSample (ch);
                delayLine.pushSample (ch, x[n] + y * fbData[n]);
                x[n] = y;
            }
        }
    }

    dryWet.mixWetSamples (block);
    outputBuffers.getReference (0) = &buffer;
}

template <typename DelayType>
void DelayModule::processPingPongDelay (AudioBuffer<float>& buffer, DelayType& delayLine)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    // if we have a mono input, copy it into a stereo buffer first
    if (numChannels == 1)
    {
        stereoBuffer.setSize (2, numSamples, false, false, true);
        stereoBuffer.clear();

        stereoBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
        stereoBuffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    auto& bufferToProcess = numChannels == 2 ? buffer : stereoBuffer;

    dsp::AudioBlock<float> block { bufferToProcess };
    dryWetMixer.setWetMixProportion (*mixParam * 0.5f);
    dryWetMixer.pushDrySamples (block);

    // if we have a stereo input collapse to mono
    if (numChannels == 2)
    {
        bufferToProcess.addFrom (0, 0, bufferToProcess, 1, 0, numSamples);
        bufferToProcess.applyGain (0, 0, numSamples, 0.5f);
    }

    auto* dataL = bufferToProcess.getWritePointer (0);
    auto* dataR = bufferToProcess.getWritePointer (1);

    const auto* fbData = feedbackSmoothBuffer.getSmoothedBuffer();
    auto processSample = [&] (int n)
    {
        auto yL = delayLine.popSample (0);
        auto yR = delayLine.popSample (1);

        auto xL = dataL[n] + dataR[n] + fbData[n] * yR;
        auto xR = fbData[n] * yL;

        delayLine.pushSample (0, xL);
        delayLine.pushSample (1, xR);

        dataL[n] = yL;
        dataR[n] = yR;
    };

    if (delaySmooth.isSmoothing() || freqSmooth.isSmoothing())
    {
        for (int n = 0; n < numSamples; ++n)
        {
            delayLine.setDelay (delaySmooth.getNextValue());
            delayLine.setFilterFreq (freqSmooth.getNextValue());
            processSample (n);
        }
    }
    else
    {
        delayLine.setDelay (delaySmooth.getTargetValue());
        delayLine.setFilterFreq (freqSmooth.getTargetValue());

        for (int n = 0; n < numSamples; ++n)
            processSample (n);
    }

    dryWetMixer.mixWetSamples (block);

    // tell future processors to use stereo buffer
    outputBuffers.getReference (0) = &bufferToProcess;
}

void DelayModule::processAudio (AudioBuffer<float>& buffer)
{
    feedbackSmoothBuffer.process (std::pow (feedbackParam->getCurrentValue() * 0.67f, 0.9f), buffer.getNumSamples());
    auto tempoSync = (int)*tempoSyncOnOffParam;
    if (!tempoSync)
    {
        std::cout << "Delay Time In Ms..." << std::endl;
        delaySmooth.setTargetValue (fs * *delayTimeMsParam * 0.001f); //delay time in samples (if tempo-sync change the calculation here?)
    }
    else
    {
        float delayInSamples = fs * 200 * 0.001f; //fallback delay
        std::cout << "Delay Time In Notes..." << std::endl;
        auto positionInfo = audioPlayHead.getPosition();
//        auto hardcodedBpm = 120;
        auto bpm = positionInfo->getBpm();
        auto noteDivision = (int)*delayTimeTempoSyncParam;
        if (noteDivision == 0)
        {
            delayInSamples = calculateTempoSyncDelayTime(2.0f, *bpm);
            std::cout << "1/2 Note Delay..." << std::endl;
        }
        else if (noteDivision == 1)
        {
            delayInSamples = calculateTempoSyncDelayTime(1.0f, *bpm);
            std::cout << "1/4 Note Delay..." << std::endl;
        }
        else if (noteDivision == 2)
        {
            delayInSamples = calculateTempoSyncDelayTime (0.5f, *bpm);
            std::cout << "1/8 Note Delay..." << std::endl;
        }
        else if (noteDivision == 3)
        {
            delayInSamples = calculateTempoSyncDelayTime (0.75f, *bpm);
            std::cout << "1/8 Note Dotted Delay..." << std::endl;
        }
        delaySmooth.setTargetValue (delayInSamples);
    }
    freqSmooth.setTargetValue (*freqParam);

    const auto delayTypeIndex = (int) *delayTypeParam;
    if (delayTypeIndex != prevDelayTypeIndex)
    {
        cleanDelayLine.reset();
        lofiDelayLine.reset();

        prevDelayTypeIndex = delayTypeIndex;
    }

    if (*pingPongParam == 0.0f)
    {
        if (delayTypeIndex == 0)
            processMonoStereoDelay (buffer, cleanDelayLine);
        else if (delayTypeIndex == 1)
            processMonoStereoDelay (buffer, lofiDelayLine);
    }
    else
    {
        if (delayTypeIndex == 0)
            processPingPongDelay (buffer, cleanDelayLine);
        else if (delayTypeIndex == 1)
            processPingPongDelay (buffer, lofiDelayLine);
    }

    bypassNeedsReset = true;
}

void DelayModule::processAudioBypassed (AudioBuffer<float>& buffer)
{
    if (bypassNeedsReset)
    {
        cleanDelayLine.reset();
        lofiDelayLine.reset();
        stereoBuffer.clear();

        bypassNeedsReset = false;
    }

    outputBuffers.getReference (0) = &buffer;
}

float DelayModule::calculateTempoSyncDelayTime(const float noteDuration, const double bpm) const
{
    //calculate tempo sync delay in samples
    auto beatsPerSecond = static_cast<float> (bpm / 60);
    auto secondsPerBeat = 1/beatsPerSecond;

    return floor(noteDuration * secondsPerBeat * fs);
}

bool DelayModule::getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp)
{
    using namespace chowdsp::ParamUtils;
    class DelayTimeModeControl : public Slider
    {
    public:
        DelayTimeModeControl (AudioProcessorValueTreeState& vtState, chowdsp::HostContextProvider& hcp)
            : vts (vtState),
              tempoSyncSelectorAttach (vts, tempoSyncAmountTag, tempoSyncSelector),
              delayTimeSlider (*getParameterPointer<chowdsp::FloatParameter*> (vts, delayTimeMsTag), hcp),
              delayTimeAttach(vts, delayTimeMsTag, delayTimeSlider),
              tempoSyncOnOffAttach (
                  *vts.getParameter (tempoSyncTag),
                  [this] (float newValue)
                  { updateControlVisibility (newValue == 1.0f); },
                  vts.undoManager)
        {
            addChildComponent (tempoSyncSelector);
            addChildComponent (delayTimeSlider);

            const auto* modeChoiceParam = getParameterPointer<AudioParameterChoice*> (vts, tempoSyncAmountTag);
            tempoSyncSelector.addItemList (modeChoiceParam->choices, 1);
            tempoSyncSelector.setSelectedItemIndex (0);
            tempoSyncSelector.setScrollWheelEnabled (true);
            hcp.registerParameterComponent (tempoSyncSelector, *modeChoiceParam);

            hcp.registerParameterComponent (delayTimeSlider, delayTimeSlider.getParameter());

            this->setName (tempoSyncAmountTag + "__" + delayTimeMsTag + "__");
        }

        void colourChanged() override
        {
            for (auto colourID : { Slider::textBoxOutlineColourId,
                                   Slider::textBoxTextColourId,
                                   Slider::textBoxBackgroundColourId,
                                   Slider::textBoxHighlightColourId,
                                   Slider::thumbColourId })
            {
                delayTimeSlider.setColour (colourID, findColour (colourID, false));
            }

            for (auto colourID : { ComboBox::outlineColourId,
                                   ComboBox::textColourId,
                                   ComboBox::arrowColourId })
            {
                tempoSyncSelector.setColour (colourID, findColour (Slider::textBoxTextColourId, false));
            }
        }

        void updateControlVisibility (bool tempoSyncOn)
        {
            tempoSyncSelector.setVisible (tempoSyncOn);
            delayTimeSlider.setVisible (!tempoSyncOn);

            setName (vts.getParameter (tempoSyncOn ? tempoSyncAmountTag : delayTimeMsTag)->name);
            if (auto* parent = getParentComponent())
                parent->repaint();
        }

        void visibilityChanged() override
        {
            updateControlVisibility (vts.getRawParameterValue (tempoSyncTag)->load() == 1.0f);
        }

        void resized() override
        {
            delayTimeSlider.setSliderStyle (getSliderStyle());
            delayTimeSlider.setTextBoxStyle (getTextBoxPosition(), false, getTextBoxWidth(), getTextBoxHeight());

            const auto bounds = getLocalBounds();
            tempoSyncSelector.setBounds (bounds.proportionOfWidth(0.15f),
                                         bounds.proportionOfHeight(0.1f),
                                         bounds.proportionOfWidth(0.7f),
                                         bounds.proportionOfHeight(0.25f));
            delayTimeSlider.setBounds (bounds);
        }

    private:
        using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
        using BoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;

        AudioProcessorValueTreeState& vts;

        ComboBox tempoSyncSelector;
        BoxAttachment tempoSyncSelectorAttach;

        ModulatableSlider delayTimeSlider;
        SliderAttachment delayTimeAttach;

        ParameterAttachment tempoSyncOnOffAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayTimeModeControl)
    };

    customComps.add (std::make_unique<DelayTimeModeControl> (vts, hcp));

    return false;
}