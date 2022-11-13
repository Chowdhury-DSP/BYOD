#pragma once

#include "../BaseProcessor.h"

class AmpIRs : public BaseProcessor, private AudioProcessorValueTreeState::Listener
{
public:
    explicit AmpIRs (UndoManager* um = nullptr);
    ~AmpIRs() override;

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void parameterChanged (const String& parameterID, float newValue) final;
    void loadIRFromStream (std::unique_ptr<InputStream>&& stream);

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, HostContextProvider& hcp) override;

    std::unique_ptr<XmlElement> toXML() override;
    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    void setMakeupGain (float irSampleRate);

    chowdsp::FloatParameter* mixParam = nullptr;
    chowdsp::FloatParameter* gainParam = nullptr;

    using IRType = std::pair<void*, size_t>;
    std::unordered_map<String, IRType> irMap;

    dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 256 } };
    dsp::Gain<float> gain;
    std::atomic<float> makeupGainDB { 0.0f };

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;
    float fs = 48000.0f;

    Array<File> irFiles;
    File curFile = File();
    CriticalSection irMutex;
    chowdsp::Broadcaster<void()> irChangedBroadcaster;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpIRs)
};
