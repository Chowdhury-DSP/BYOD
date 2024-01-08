#pragma once

#include "processors/BaseProcessor.h"

class FrequencyShifter : public BaseProcessor
{
public:
    explicit FrequencyShifter (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Modulation; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;
    void processAudioBypassed (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider& hcp) override;

	enum class Mode
	{
		Plain = 1,
		Stereo = 2,
		Spread = 4,
	};

private:
	void processPlainOrStereo (chowdsp::BufferView<const float> bufferIn,
	                           chowdsp::BufferView<float> bufferOut,
	                           bool isStereo) noexcept;
	void processSpread (chowdsp::BufferView<const float> bufferIn,
	                    chowdsp::BufferView<float> bufferOut) noexcept;

    chowdsp::FloatParameter* shiftParam = nullptr;
	chowdsp::SmoothedBufferValue<float> feedback {};
    chowdsp::FloatParameter* spreadParam = nullptr;
    chowdsp::SmoothedBufferValue<float> dryGain, wetGain;
	chowdsp::EnumChoiceParameter<Mode>* modeParam = nullptr;

	juce::AudioBuffer<float> outputBuffer;
	juce::AudioBuffer<float> dryBuffer;

    std::array<chowdsp::HilbertFilter<float>, 3> hilbertFilter;
    std::array<chowdsp::SineWave<float>, 3> modulators {};

	chowdsp::FirstOrderHPF<float> dcBlocker;

	std::array<float, 3> fbData {};
	std::vector<float> lfoData;

	Mode prevMode = Mode::Plain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyShifter)
};
