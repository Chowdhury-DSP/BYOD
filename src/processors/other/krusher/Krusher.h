#pragma once

#include "processors/BaseProcessor.h"

#define KRUSHER_USE_JAI_IMPL ! JUCE_ARM&& BYOD_BUILDING_JAI_MODULES

#if KRUSHER_USE_JAI_IMPL
#include "jai/byod_jai_lib.h"
#else
#include "krusher_fallback_impl.h"
#endif

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

#if KRUSHER_USE_JAI_IMPL
    SharedJaiContext jai_context;
    jai_Krusher_Lofi_Resample_State resample_state {};
    std::array<jai_Krusher_Bit_Reducer_Filter_State, 2> brFilterStates {};
#else
    std::unique_ptr<chowdsp::NullType> jai_context;
    Krusher_Lofi_Resample_State resample_state {};
    std::array<Krusher_Bit_Reducer_Filter_State, 2> brFilterStates {};
#endif

    chowdsp::FirstOrderHPF<float> dcBlocker;

    chowdsp::Gain<float> wetGain;
    chowdsp::Gain<float> dryGain;
    chowdsp::Buffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Krusher)
};
