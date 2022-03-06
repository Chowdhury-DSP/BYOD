#pragma once

#include "../BaseProcessor.h"
#include "gui/utils/LevelMeterComponent.h"

class OutputProcessor : public BaseProcessor
{
public:
    explicit OutputProcessor (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void resetLevels();
    void processAudio (AudioBuffer<float>& buffer) override;

    void getCustomComponents (OwnedArray<Component>& customComps) override;

private:
    LevelMeterComponent::LevelDataType rmsLevels {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputProcessor)
};
