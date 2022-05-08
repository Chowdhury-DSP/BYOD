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

private:
    AudioProcessorValueTreeState& vts;
    ProcessorChain& procChain;
    chowdsp::PresetManager& presetManager;

    chowdsp::UIState uiState;

    static const Identifier stateTag;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateManager)
};
