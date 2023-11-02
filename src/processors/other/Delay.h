#pragma once

#include "../BaseProcessor.h"
#include "processors/PlayheadHelpers.h"

class DelayModule : public BaseProcessor
{
public:
    explicit DelayModule (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();
    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

    void prepare (double sampleRate, int samplesPerBlock) override;
    void releaseMemory() override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;
    float calculateTempoSyncDelayTime(const double timeInSeconds, const double sampleRate) const;

private:
    template <typename DelayType>
    void processMonoStereoDelay (AudioBuffer<float>& buffer, DelayType& delayLine);
    template <typename DelayType>
    void processPingPongDelay (AudioBuffer<float>& buffer, DelayType& delayLine);

    struct SimpleAudioPlayHead : juce::AudioPlayHead {
        juce::Optional<AudioPlayHead::PositionInfo> getPosition() const override {
            PositionInfo info;
            return info;
        }
    };

    SimpleAudioPlayHead audioPlayHead;

    chowdsp::FloatParameter* freqParam = nullptr;
    chowdsp::FloatParameter* feedbackParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;
    std::atomic<float>* delayTypeParam = nullptr;
    std::atomic<float>* pingPongParam = nullptr;

    chowdsp::FloatParameter* delayTimeMsParam = nullptr;
    std::atomic<float>* delayTimeTempoSyncParam = nullptr;
    std::atomic<float>* tempoSyncOnOffParam = nullptr;

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;

    struct CleanDelayType
    {
        void prepare (dsp::ProcessSpec& spec)
        {
            lpf.prepare (spec);
            delay.prepare (spec);
        }

        void reset()
        {
            lpf.reset();
            delay.reset();
        }

        void setDelay (float newDelayInSamples) { delay.setDelay (newDelayInSamples); }
        void setFilterFreq (float freqHz) { lpf.setCutoffFrequency (freqHz); }

        inline void pushSample (int channel, float sample) { delay.pushSample (channel, sample); }
        inline float popSample (int channel) { return lpf.processSample (channel, delay.popSample (channel)); }

        chowdsp::SVFLowpass<float> lpf;
        chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th> delay { 1 << 20 };
    };
    CleanDelayType cleanDelayLine;

    using LofiDelayType = chowdsp::BBD::BBDDelayWrapper<4 * 16384>;
    LofiDelayType lofiDelayLine;
    int prevDelayTypeIndex = 0;

    SmoothedValue<float, ValueSmoothingTypes::Linear> delaySmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> freqSmooth;
    chowdsp::SmoothedBufferValue<float, ValueSmoothingTypes::Linear> feedbackSmoothBuffer;

    float fs = 48000.0f;
    AudioBuffer<float> stereoBuffer;

    bool bypassNeedsReset = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayModule)
};
