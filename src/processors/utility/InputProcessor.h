#pragma once

#include "../BaseProcessor.h"
#include "gui/utils/LevelMeterComponent.h"

class InputProcessor : public BaseProcessor
{
public:
    explicit InputProcessor (UndoManager* um = nullptr);

    static constexpr std::string_view name = "Input";

    ProcessorType getProcessorType() const override { return Utility; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void resetLevels();
    void processAudio (AudioBuffer<float>& buffer) override;

    bool getCustomComponents (OwnedArray<Component>& customComps, chowdsp::HostContextProvider&) override;

private:
    LevelMeterComponent::LevelDataType rmsLevels {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputProcessor)
};
