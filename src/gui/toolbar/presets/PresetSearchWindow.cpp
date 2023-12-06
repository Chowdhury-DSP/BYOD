#include "PresetSearchWindow.h"

namespace PresetSearchDims
{
constexpr int rowHeight = 40;
constexpr int itemHeight = rowHeight + 20;
} // namespace PresetSearchDims

struct PresetSearchWindow::ResultsListModel : public ListBoxModel
{
    explicit ResultsListModel (preset_search::Results&& results) : searchResults (std::move (results))
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
        const auto* result = searchResults[(int) rowNumber];
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

        const auto* result = searchResults[(int) rowNumber];
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

    const preset_search::Results searchResults;
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
    juce::Component::setName ("Presets Search");

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
    resultsBox.setRowHeight (PresetSearchDims::rowHeight);
    addAndMakeVisible (resultsBox);

    numResultsLabel.setColour (Label::backgroundColourId, Colours::transparentBlack);
    numResultsLabel.setColour (Label::outlineColourId, Colours::transparentBlack);
    numResultsLabel.setJustificationType (Justification::left);
    addAndMakeVisible (numResultsLabel);

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

    searchEntryBox->setFont ((float) PresetSearchDims::rowHeight * 0.55f);
    searchEntryBox->setBounds (bounds.removeFromTop (PresetSearchDims::itemHeight).reduced (10, 10));
    resultsBox.setBounds (bounds.reduced (10, 0));
    numResultsLabel.setBounds (footer.reduced (10, 1));
}

void PresetSearchWindow::updatePresetSearchDatabase()
{
    preset_search::initialiseDatabase (presetManager, searchDatabase);
    searchEntryBox->setText ({}, juce::sendNotification);
    updateSearchResults ({});
}

void PresetSearchWindow::updateSearchResults (const String& searchQuery)
{
    resultsBoxModel = std::make_unique<ResultsListModel> (preset_search::getSearchResults (presetManager, searchDatabase, searchQuery));
    resultsBox.setModel (resultsBoxModel.get());
    numResultsLabel.setText ("Found: " + String (resultsBoxModel->getNumRows()) + " presets", sendNotificationSync);

    resultsBoxModel->loadPresetCallback = [&] (const chowdsp::Preset* preset)
    {
        presetManager.loadPreset (*preset);
    };
}
