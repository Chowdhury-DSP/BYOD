#pragma once

#include "../neural_utils/ResampledRNN.h"
#include "../neural_utils/SampleGRU.h"

class GainStageML
{
public:
    GainStageML (AudioProcessorValueTreeState& vts);

    void reset (double sampleRate, int samplesPerBlock);
    void processBlock (AudioBuffer<float>& buffer);

private:
    enum
    {
        numModels = 5,
    };

    using RNNModel = ResampledRNN<8, RTNeural::GRULayerT>;
    using ModelPair = std::array<RNNModel, 2>;
    std::array<ModelPair, numModels> gainStageML;

    static void loadModel (ModelPair& model, const char* data, int size);
    static void processModel (AudioBuffer<float>& buffer, ModelPair& model);

    inline int getModelIdx() const noexcept
    {
        return jlimit (0, numModels - 1, int ((float) numModels * *gainParam));
    }

    AudioBuffer<float> fadeBuffer;
    std::atomic<float>* gainParam = nullptr;
    int lastModelIdx = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainStageML)
};
