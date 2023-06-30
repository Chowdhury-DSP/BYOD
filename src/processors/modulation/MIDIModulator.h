#pragma once

#include "processors/BaseProcessor.h"

class MidiModulator : public BaseProcessor
{
public:
    explicit MidiModulator (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&) override;

    std::unique_ptr<XmlElement> toXML() override;
    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    chowdsp::BoolParameter* bipolarParam = nullptr;

    chowdsp::SmoothedBufferValue<float> midiModSmooth;
    std::atomic_int modControlValue { 0 };
    int mappedModController = 1;
    std::atomic_bool isLearning { false };

    AudioBuffer<float> modOutBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiModulator)
};
