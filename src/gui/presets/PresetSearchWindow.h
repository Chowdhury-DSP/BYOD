#pragma once

#include "gui/utils/LabelWithCentredEditor.h"

class PresetSearchWindow : public Component
{
public:
    explicit PresetSearchWindow (chowdsp::PresetManager& presetManager);
    ~PresetSearchWindow() override;

    void paint (Graphics& g) override;
    void resized() override;

    using ResultsVec = std::vector<std::pair<const chowdsp::Preset*, double>>;

private:
    void updateSearchResults (const String& searchQuery);
    void setUpListModel (ResultsVec&& results);

    chowdsp::PresetManager& presetManager;

    struct SearchLabel;
    std::unique_ptr<SearchLabel> searchEntryBox;
    
    struct ResultsListModel;
    std::unique_ptr<ResultsListModel> resultsBoxModel;
    ListBox resultsBox;

    std::vector<const chowdsp::Preset*> allPresetsVector;

    Label numResultsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSearchWindow)
};
