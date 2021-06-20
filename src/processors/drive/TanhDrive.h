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

        return { params.begin(), params.end() };
    }

    virtual ProcessorType getProcessorType() const { return ProcessorType::Drive; }

    virtual void prepare (double sampleRate, int samplesPerBlock) 
    {
        // @TODO...
    }

    virtual void processAudio (AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n , buffer.getNumSamples(); ++ch)
                x[n] = std::tanh (x[n]);
        }
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TanhDrive)
};
