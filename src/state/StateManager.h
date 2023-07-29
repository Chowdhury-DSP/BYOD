#pragma once

#include "processors/chain/ProcessorChain.h"

class StateManager
{
public:
    StateManager (AudioProcessorValueTreeState& vts,
                  ProcessorChain& procChain,
                  chowdsp::PresetManager& presetManager);

    std::unique_ptr<XmlElement> saveState();
    void loadState (XmlElement* xml);

    auto& getUIState() { return uiState; }

    static void setCurrentPluginVersionInXML (XmlElement* xml);
    static chowdsp::Version getPluginVersionFromXML (const XmlElement* xml);

private:
    AudioProcessorValueTreeState& vts;
    ProcessorChain& procChain;
    chowdsp::PresetManager& presetManager;

    chowdsp::UIState uiState;
    const juce::AudioProcessor::WrapperType pluginWrapperType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateManager)
};
