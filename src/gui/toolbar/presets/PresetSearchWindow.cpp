#include "PresetSearchWindow.h"

namespace
{
constexpr int rowHeight = 40;
constexpr int itemHeight = rowHeight + 20;
} // namespace

struct PresetSearchWindow::ResultsListModel : public ListBoxModel
{
    using ConstResultsVec = const PresetSearchWindow::ResultsVec;

    explicit ResultsListModel (PresetSearchWindow::ResultsVec&& results) : searchResults (std::move (results))
    {
    }

    std::function<void (const chowdsp::Preset*)> loadPresetCallback {};

    int getNumRows() override
    {
        return (int) searchResults.size();
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { loadPresetForRow (row); }
    void returnKeyPressed (int row) override { loadPresetForRow (row); }

    void loadPresetForRow (int rowNumber)
    {
        const auto* result = searchResults[(int) rowNumber].first;
        if (result == nullptr)
        {
            jassertfalse;
            return;
        }

        loadPresetCallback (result);
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto bounds = Rectangle { width, height };
        if (rowIsSelected)
        {
            g.setColour (Colour (0xFF0EDED4));
            g.fillRect (bounds);
        }

        g.setColour (Colours::white.withAlpha (0.4f));
        g.drawLine (Line { bounds.getTopLeft(), bounds.getTopRight() }.toFloat(), 1.0f);
        if (rowNumber >= (int) searchResults.size() - 1)
            g.drawLine (Line { bounds.getBottomLeft(), bounds.getBottomRight() }.toFloat(), 1.0f);

        const auto* result = searchResults[(int) rowNumber].first;
        if (result == nullptr)
        {
            jassertfalse;
            return;
        }

        const auto name = result->getName();
        const auto vendor = result->getVendor();
        const auto category = result->getCategory();

        g.setFont ((float) height * 0.55f);
        g.setColour (rowIsSelected ? Colours::black : Colours::white);
        g.drawFittedText (name, bounds, Justification::centred, 1);

        g.setFont ((float) height * 0.35f);
        const auto vendorBounds = bounds.removeFromLeft (width / 2).withTop (height / 2).reduced (5);
        g.drawFittedText (vendor, vendorBounds, Justification::left, 1);

        const auto categoryBounds = bounds.withTop (height / 2).reduced (5);
        g.drawFittedText (category, categoryBounds, Justification::right, 1);
    }

    ConstResultsVec searchResults;
};

struct PresetSearchWindow::SearchLabel : LabelWithCentredEditor
{
    void textEditorTextChanged (TextEditor& editor) override
    {
        labelChangeCallback (editor.getText());
    }

    std::function<void (const String&)> labelChangeCallback = [] (const String&) {};
};

PresetSearchWindow::PresetSearchWindow (chowdsp::PresetManager& presetMgr) : presetManager (presetMgr)
{
    setName ("Presets Search");

    auto setupLabel = [&] (auto& label)
    {
        label = std::make_unique<SearchLabel>();
        label->setColour (Label::backgroundColourId, Colours::transparentBlack);
        label->setColour (Label::outlineColourId, Colours::white);

        label->setJustificationType (Justification::centred);
        label->setEditable (true);

        addAndMakeVisible (*label);
    };

    setupLabel (searchEntryBox);
    searchEntryBox->labelChangeCallback = [&] (const String& searchQuery)
    { updateSearchResults (searchQuery); };

    resultsBox.setColour (ListBox::outlineColourId, Colours::white);
    resultsBox.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    resultsBox.setOutlineThickness (1);
    resultsBox.setRowHeight (rowHeight);
    addAndMakeVisible (resultsBox);

    numResultsLabel.setColour (Label::backgroundColourId, Colours::transparentBlack);
    numResultsLabel.setColour (Label::outlineColourId, Colours::transparentBlack);
    numResultsLabel.setJustificationType (Justification::left);
    addAndMakeVisible (numResultsLabel);

    updateSearchResults (String());

    setSize (600, 400);
}

PresetSearchWindow::~PresetSearchWindow() = default;

void PresetSearchWindow::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void PresetSearchWindow::resized()
{
    auto bounds = getLocalBounds();
    const auto footer = bounds.removeFromBottom (20);

    searchEntryBox->setFont ((float) rowHeight * 0.55f);
    searchEntryBox->setBounds (bounds.removeFromTop (itemHeight).reduced (10, 10));
    resultsBox.setBounds (bounds.reduced (10, 0));
    numResultsLabel.setBounds (footer.reduced (10, 1));
}

void PresetSearchWindow::updateSearchResults (const String& searchQuery)
{
    if (searchQuery.isEmpty())
    {
        ResultsVec resultsVector;
        resultsVector.reserve ((size_t) presetManager.getNumPresets());
        for (const auto& [_, preset] : presetManager.getPresetMap())
            resultsVector.emplace_back (&preset, 0.0);

        setUpListModel (std::move (resultsVector));
        return;
    }

    ResultsVec resultsVector;
    const auto searchQueryStr = searchQuery.toStdString();
    constexpr double scoreThreshold = 35.0;
    auto searchScorer = rapidfuzz::fuzz::CachedRatio<char> (searchQueryStr);
    for (const auto& [_, preset] : presetManager.getPresetMap())
    {
        auto score = searchScorer.similarity (preset.getName().toStdString());
        if (score > scoreThreshold)
            resultsVector.emplace_back (&preset, score);
    }

    std::sort (resultsVector.begin(), resultsVector.end(), [] (const auto& a, const auto& b)
               { return a.second > b.second; });
    setUpListModel (std::move (resultsVector));
}

void PresetSearchWindow::setUpListModel (ResultsVec&& results)
{
    resultsBoxModel = std::make_unique<ResultsListModel> (std::move (results));
    resultsBox.setModel (resultsBoxModel.get());
    numResultsLabel.setText ("Found: " + String (resultsBoxModel->getNumRows()) + " presets", sendNotificationSync);

    resultsBoxModel->loadPresetCallback = [&] (const chowdsp::Preset* preset)
    {
        presetManager.loadPreset (*preset);
    };
}
