#pragma once

#include "DiodeClipperWDF.h"
#include "processors/BaseProcessor.h"

class DiodeClipper : public BaseProcessor
{
public:
    explicit DiodeClipper (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void setGains (float driveValue);

private:
    chowdsp::FloatParameter* cutoffParam = nullptr;
    chowdsp::FloatParameter* driveParam = nullptr;
    std::atomic<float>* diodeTypeParam = nullptr;
    chowdsp::FloatParameter* nDiodesParam = nullptr;

    dsp::Gain<float> inGain, outGain;
    using DiodeClipperDP = DiodeClipperWDF<wdft::DiodePairT>;
    DiodeClipperDP wdf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeClipper)
};
