#pragma once

#include "gui/utils/LabelWithCentredEditor.h"
#include "PresetSearchHelpers.h"

class PresetSearchWindow : public Component
{
public:
    explicit PresetSearchWindow (chowdsp::PresetManager& presetManager);
    ~PresetSearchWindow() override;

    void paint (Graphics& g) override;
    void resized() override;

    void updatePresetSearchDatabase();

private:
    void updateSearchResults (const String& searchQuery);

    chowdsp::PresetManager& presetManager;
    preset_search::Database searchDatabase;

    struct SearchLabel;
    std::unique_ptr<SearchLabel> searchEntryBox;

    struct ResultsListModel;
    std::unique_ptr<ResultsListModel> resultsBoxModel;
    ListBox resultsBox;

    Label numResultsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSearchWindow)
};
