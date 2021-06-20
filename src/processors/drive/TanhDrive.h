#pragma once

#include "../BaseProcessor.h"

class TanhDrive : public BaseProcessor
{
public:
    TanhDrive (UndoManager* um = nullptr) : BaseProcessor ("TanhDrive", createParameterLayout(), um)
    {

    }

    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using Params = std::vector<std::unique_ptr<RangedAudioParameter>>;
        Params params;

        params.push_back (std::make_unique<AudioParameterFloat> ("float_param", "Float", 0.0f, 1.0f, 0.5f));
        params.push_back (std::make_unique<AudioParameterBool> ("bool_param2", "Bool", false));
        params.push_back (std::make_unique<AudioParameterChoice> ("choice_param3", "Choice", StringArray { "One", "Two" }, 1));

        return { params.begin(), params.end() };
    }

    virtual ProcessorType getProcessorType() const { return ProcessorType::Drive; }

    virtual void prepare (double, int) 
    {
        // @TODO...
    }

    virtual void processAudio (AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                x[n] = std::tanh (x[n]);
        }
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TanhDrive)
};
