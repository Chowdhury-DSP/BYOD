#pragma once

#include "../BaseProcessor.h"
#include "gui/utils/LevelMeterComponent.h"

class InputProcessor : public BaseProcessor
{
public:
    explicit InputProcessor (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

    void getCustomComponents (OwnedArray<Component>& customComps) override;

private:
    std::atomic<float>* monoModeParam = nullptr;

    AudioBuffer<float> monoBuffer;

    LevelMeterComponent::LevelDataType rmsLevels {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputProcessor)
};
