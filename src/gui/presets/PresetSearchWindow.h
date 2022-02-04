#pragma once

#include "gui/utils/LabelWithCentredEditor.h"

class PresetSearchWindow : public Component
{
public:
    explicit PresetSearchWindow (chowdsp::PresetManager& presetManager);

    void paint (Graphics& g) override;
    void resized() override;

    using ResultsVec = std::vector<const chowdsp::Preset*>;

private:
    chowdsp::PresetManager& presetManager;

    LabelWithCentredEditor searchEntryBox;
    ListBox resultsBox;
    std::unique_ptr<ListBoxModel> resultsBoxModel;

    ResultsVec resultsVector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSearchWindow)
};
