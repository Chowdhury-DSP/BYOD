#pragma once

#include <pch.h>

class ProcessorChain;
class PresetManager : public chowdsp::PresetManager
{
public:
    PresetManager (ProcessorChain* chain, AudioProcessorValueTreeState& vts);

    std::unique_ptr<XmlElement> savePresetState() override;
    void loadPresetState (const XmlElement* xml) override;

private:
    ProcessorChain* procChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
