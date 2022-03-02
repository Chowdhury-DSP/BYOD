#pragma once

#include "../BaseProcessor.h"

class AmpIRs : public BaseProcessor, private AudioProcessorValueTreeState::Listener
{
    CREATE_LISTENER (
        AmpIRListener,
        listeners,
        virtual void irChanged() {})
public:
    explicit AmpIRs (UndoManager* um = nullptr);
    ~AmpIRs() override;

    ProcessorType getProcessorType() const override { return Tone; }
    static ParamLayout createParameterLayout();

    void parameterChanged (const String& parameterID, float newValue) final;
    void loadIRFromFile (const File& file);

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    void getCustomComponents (OwnedArray<Component>& customComps) override;

    std::unique_ptr<XmlElement> toXML() override;
    void fromXML (XmlElement* xml, bool loadPosition) override;

private:
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    using IRType = std::pair<void*, size_t>;
    std::unordered_map<String, IRType> irMap;

    dsp::Convolution convolution;
    dsp::Gain<float> gain;

    dsp::DryWetMixer<float> dryWetMixer;
    dsp::DryWetMixer<float> dryWetMixerMono;

    Array<File> irFiles;
    File curFile = File();
    CriticalSection irMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpIRs)
};
