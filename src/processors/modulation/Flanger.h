#pragma once

#include "processors/BaseProcessor.h"

class Flanger : public BaseProcessor
{
public:
    explicit Flanger (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

private:
//    template <typename DelayArrType>
//    void processChorus (AudioBuffer<float>& buffer, DelayArrType& delay);
//    void processModulation (int numSamples);
//
    chowdsp::FloatParameter* rateParam = nullptr;
    chowdsp::FloatParameter* fbParam = nullptr;
    chowdsp::FloatParameter* delayAmountParam = nullptr;
    chowdsp::FloatParameter* delayOffsetParam = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;
    
    dsp::DryWetMixer<float> dryWetMixer;//remove me?

    
    
    
    
    std::atomic<float>* delayTypeParam = nullptr;
//
//
//
//    static constexpr int delaysPerChannel = 2;
//
//    SmoothedValue<float, ValueSmoothingTypes::Linear> slowSmooth[2];
//    SmoothedValue<float, ValueSmoothingTypes::Linear> fastSmooth[2];
//
//    chowdsp::SineWave<float> slowLFOs[2][delaysPerChannel];
//    chowdsp::SineWave<float> fastLFOs[2][delaysPerChannel];
//
//    std::vector<float> slowLFOData[2][delaysPerChannel];
//    std::vector<float> fastLFOData[2][delaysPerChannel];
//    chowdsp::HilbertFilter<float> hilbertFilter[2];
//
//    struct CleanDelayType
//    {
//        void prepare (const dsp::ProcessSpec& spec)
//        {
//            lpf.prepare (spec);
//            delay.prepare (spec);
//        }
//
//        void reset()
//        {
//            lpf.reset();
//            delay.reset();
//        }
//
//        void setDelay (float newDelayInSamples) { delay.setDelay (newDelayInSamples); }
//        void setFilterFreq (float freqHz) { lpf.setCutoffFrequency (freqHz); }
//
//        inline void pushSample (int channel, float sample) { delay.pushSample (channel, sample); }
//        inline float popSample (int channel) { return lpf.processSample (channel, delay.popSample (channel)); }
//
//        chowdsp::SVFLowpass<float> lpf;
        chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th> delay { 1 << 18 };
//    };
//
//    template <typename DelayType>
//    using DelaySet = std::array<std::array<DelayType, delaysPerChannel>, 2>;
//    DelaySet<CleanDelayType> cleanDelay;
//
//    using LofiDelayType = chowdsp::BBD::BBDDelayWrapper<1024>;
//    DelaySet<LofiDelayType> lofiDelay;
//    int prevDelayTypeIndex = 0;
//
//    chowdsp::SVFLowpass<float> aaFilter;
//
//    float feedbackState[2] { 0.0f, 0.0f };
//    SmoothedValue<float, ValueSmoothingTypes::Linear> fbSmooth[2];
//    chowdsp::SVFHighpass<float> dcBlocker;

    float fs = 48000.0f;
    AudioBuffer<float> audioOutBuffer;
    AudioBuffer<float> modOutBuffer;

    bool bypassNeedsReset = false;

    enum InputPort
    {
        AudioInput = 0,
        ModulationInput,
    };

    enum OutputPort
    {
        AudioOutput = 0,
        ModulationOutput,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Flanger)
};

