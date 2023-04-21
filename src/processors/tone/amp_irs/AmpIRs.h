#pragma once

#include "processors/BaseProcessor.h"

class AmpIRs : public BaseProcessor, private AudioProcessorValueTreeState::Listener
{
public:
    explicit AmpIRs (UndoManager* um = nullptr);
    ~AmpIRs() override;

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void parameterChanged (const String& parameterID, float newValue) final;
    void loadIRFromStream (std::unique_ptr<InputStream>&& stream, const String& name = {}, Component* associatedComp = nullptr);

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

    std::unique_ptr<XmlElement> toXML() override;
    void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition) override;

private:
    void loadIRFromCurrentState();
    void setMakeupGain (float irSampleRate);

    chowdsp::FloatParameter* mixParam = nullptr;
    chowdsp::FloatParameter* gainParam = nullptr;

    dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 256 } };
    dsp::Gain<float> gain;
    std::atomic<float> makeupGainDB { 0.0f };

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;
    float fs = 48000.0f;

    using IRType = std::pair<void*, size_t>;
    std::unordered_map<String, IRType> irMap; // store the IRs that come from BinaryData

    struct IRState
    {
        // name should always be set
        // file might be set
        // if paramIndex == customIRIndex, then the data must exist
        // if paramIndex >= 0, then data should be NULL
        String name {};
        File file {};
        int paramIndex = -1;
        std::unique_ptr<MemoryBlock> data {};
    };

    IRState irState;
    CriticalSection irMutex;
    chowdsp::Broadcaster<void()> irChangedBroadcaster;
    AudioFormatManager audioFormatManager;

    inline static const StringArray irNames {
        "Fender",
        "Marshall",
        "Bogner",
        "Bass",
        "Custom",
    };

    inline static const String irTag = "ir";
    inline static const String mixTag = "mix";
    inline static const String gainTag = "gain";
    inline static const int customIRIndex = irNames.indexOf ("Custom");

    friend struct AmpIRsSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpIRs)
};
