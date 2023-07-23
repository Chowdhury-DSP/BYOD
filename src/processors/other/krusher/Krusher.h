#pragma once

#include "processors/BaseProcessor.h"

#include "jai/byod_jai_lib.h"

//#include "BRRHelpers.h"
//#include "LoFiDownsampler.h"

class Krusher : public BaseProcessor
{
public:
    explicit Krusher (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    void processDownsampler (const chowdsp::BufferView<float>& buffer, float targetFs, bool antialias) noexcept;

    chowdsp::FloatParameter* sampleRateParam = nullptr;
    chowdsp::BoolParameter* antialiasParam = nullptr;
    chowdsp::FloatParameter* bitDepthParam = nullptr;
    chowdsp::ChoiceParameter* brrFilterIndex = nullptr;
    chowdsp::FloatParameter* mixParam = nullptr;

    chowdsp::EllipticFilter<8> aaFilter;
    chowdsp::EllipticFilter<8> aiFilter;
    float hostFs = 48000.0f;

    SharedJaiContext jai_context;
    jai_Krusher_Lofi_Resample_State resample_state {};

//    LoFiDownsampler loFiDownsampler;
//
//    std::array<brr_helpers::BRRFilterState, 2> brrFilterStates {};

    chowdsp::FirstOrderHPF<float> dcBlocker;

    chowdsp::Gain<float> wetGain;
    chowdsp::Gain<float> dryGain;
    chowdsp::Buffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Krusher)
};
