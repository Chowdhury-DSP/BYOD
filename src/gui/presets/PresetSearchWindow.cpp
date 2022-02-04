#include "PresetSearchWindow.h"

namespace
{
constexpr int itemHeight = 50;
constexpr int rowHeight = 40;

struct ResultsListModel : public ListBoxModel
{
    using ConstResultsVec = const PresetSearchWindow::ResultsVec;

    explicit ResultsListModel (ConstResultsVec& results) : searchResults (results)
    {
    }

    int getNumRows() override
    {
        return (int) searchResults.size();
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool /*rowIsSelected*/) override
    {
        auto bounds = Rectangle { width, height };

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
        g.setColour (Colours::white);
        g.drawFittedText (name, bounds, Justification::centred, 1);

        g.setFont ((float) height * 0.35f);
        const auto vendorBounds = bounds.removeFromLeft (width / 2).withTop (height / 2).reduced (5);
        g.drawFittedText (vendor, vendorBounds, Justification::left, 1);

        const auto categoryBounds = bounds.withTop (height / 2).reduced (5);
        g.drawFittedText (category, categoryBounds, Justification::right, 1);
    }

    ConstResultsVec& searchResults;
};
} // namespace

PresetSearchWindow::PresetSearchWindow (chowdsp::PresetManager& presetMgr) : presetManager (presetMgr)
{
    auto setupLabel = [&] (auto& label)
    {
        label.setColour (Label::backgroundColourId, Colours::transparentBlack);
        label.setColour (Label::outlineColourId, Colours::white);

        label.setJustificationType (Justification::centred);
        label.setEditable (true);

        addAndMakeVisible (label);
    };

    setupLabel (searchEntryBox);

    resultsBox.setColour (ListBox::outlineColourId, Colours::white);
    resultsBox.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    resultsBox.setOutlineThickness (1);
    resultsBox.setRowHeight (rowHeight);
    addAndMakeVisible (resultsBox);

    for (const auto& [_, preset] : presetManager.getPresetMap())
        resultsVector.push_back (&preset);

    resultsBoxModel = std::make_unique<ResultsListModel> (resultsVector);
    resultsBox.setModel (resultsBoxModel.get());

    setSize (600, 400);
}

void PresetSearchWindow::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void PresetSearchWindow::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (5);
    searchEntryBox.setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5));

    resultsBox.setBounds (bounds.reduced (10, 10));
}
