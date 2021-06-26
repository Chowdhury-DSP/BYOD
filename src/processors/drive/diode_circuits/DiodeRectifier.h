#pragma once

#include "processors/BaseProcessor.h"
#include "DiodeClipperWDF.h"

class DiodeRectifier : public BaseProcessor
{
public:
    DiodeRectifier (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Drive; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void setGains (float driveValue);

private:
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* diodeTypeParam = nullptr;
    std::atomic<float>* nDiodesParam = nullptr;

    dsp::Gain<float> inGain, outGain;
    using DiodeRectifierWDF = DiodeClipperWDF<wdft::DiodeT>;
    std::unique_ptr<DiodeRectifierWDF> wdf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiodeRectifier)
};
