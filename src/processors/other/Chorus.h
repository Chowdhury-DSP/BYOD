#pragma once

#include "../BaseProcessor.h"
#include "../ParameterHelpers.h"

class Chorus : public BaseProcessor
{
public:
    explicit Chorus (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    void addToPopupMenu (PopupMenu& menu) override;

private:
    template <typename DelayArrType>
    void processChorus (AudioBuffer<float>& buffer, DelayArrType& delay);

    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* fbParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* delayTypeParam = nullptr;

    dsp::DryWetMixer<float> dryWetMixer;

    static constexpr int delaysPerChannel = 2;

    SmoothedValue<float, ValueSmoothingTypes::Linear> slowSmooth[2];
    SmoothedValue<float, ValueSmoothingTypes::Linear> fastSmooth[2];

    chowdsp::SineWave<float> slowLFOs[2][delaysPerChannel];
    chowdsp::SineWave<float> fastLFOs[2][delaysPerChannel];

    struct CleanDelayType
    {
        void prepare (const dsp::ProcessSpec& spec)
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
        inline float popSample (int channel) { return lpf.processSample<chowdsp::StateVariableFilterType::Lowpass> (channel, delay.popSample (channel)); }

        chowdsp::StateVariableFilter<float> lpf;
        chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th> delay { 1 << 18 };
    };

    template <typename DelayType>
    using DelaySet = std::array<std::array<DelayType, delaysPerChannel>, 2>;
    DelaySet<CleanDelayType> cleanDelay;

    using LofiDelayType = chowdsp::BBD::BBDDelayWrapper<1024>;
    DelaySet<LofiDelayType> lofiDelay;
    int prevDelayTypeIndex = 0;

    chowdsp::StateVariableFilter<float> aaFilter;

    float feedbackState[2] { 0.0f, 0.0f };
    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];
    chowdsp::StateVariableFilter<float> dcBlocker;

    float fs = 48000.0f;
    AudioBuffer<float> stereoBuffer;

    std::unique_ptr<ParameterAttachment> delayTypeAttach;

    bool bypassNeedsReset = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chorus)
};
