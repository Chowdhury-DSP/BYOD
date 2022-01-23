#include "PresetsSyncDialog.h"

namespace
{
constexpr int headerHeight = 40;
constexpr int footerHeight = 50;

constexpr int updatingHeight = headerHeight + footerHeight + 200;
constexpr int noUpdateHeight = headerHeight + footerHeight;
constexpr int updatingWidth = 400;
constexpr int noUpdateWidth = 300;

constexpr int rowHeight = 25;

struct PresetsListModel : public ListBoxModel
{
    explicit PresetsListModel (const std::vector<chowdsp::Preset>& presetsToUpdate)
    {
        for (const auto& preset : presetsToUpdate)
            presetNames.add (preset.getName());
    }

    int getNumRows() override
    {
        return presetNames.size();
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto bounds = Rectangle { width, height };

        g.setColour (Colours::white);
        g.setFont ((float) height * 0.6f);
        g.drawFittedText (presetNames[rowNumber], bounds, Justification::centred, 1);

        g.setColour (Colours::white.withAlpha (0.4f));
        g.drawLine (Line { bounds.getTopLeft(), bounds.getTopRight() }.toFloat(), 1.0f);
        g.drawLine (Line { bounds.getBottomLeft(), bounds.getBottomRight() }.toFloat(), 1.0f);
    }

    StringArray presetNames;
};
} // namespace

PresetsSyncDialog::PresetsSyncDialog()
{
    setName ("Preset Server Syncing");

    presetsList.setClickingTogglesRowSelection (false);
    presetsList.setRowHeight (rowHeight);
    addAndMakeVisible (presetsList);

    auto setupButton = [=] (TextButton& button)
    {
        button.setColour (TextButton::ColourIds::buttonColourId, Colours::transparentBlack);
        button.setColour (TextButton::ColourIds::textColourOffId, Colours::white);
        addAndMakeVisible (button);
    };

    setupButton (okButton);
    setupButton (cancelButton);

    cancelButton.onClick = [&]
    { getParentComponent()->setVisible (false); };
}

void PresetsSyncDialog::updatePresetsList (const std::vector<chowdsp::Preset>& presetsToUpdate)
{
    if (presetsToUpdate.empty())
    {
        presetUpdateText = "All presets are up-to-date!";
        presetsList.setModel (nullptr);

        setSize (noUpdateWidth, noUpdateHeight);

        okButton.onClick = [&]
        { getParentComponent()->setVisible (false); };

        return;
    }

    presetUpdateText = "The following presets will be updated:";
    listBoxModel = std::make_unique<PresetsListModel> (presetsToUpdate);
    presetsList.setModel (listBoxModel.get());
    setSize (updatingWidth, updatingHeight);

    okButton.onClick = [&]
    {
        runUpdateCallback();
        getParentComponent()->setVisible (false);
    };
}

void PresetsSyncDialog::paint (Graphics& g)
{
    g.fillAll (Colours::black);

    auto headerBounds = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (Colours::white);
    g.setFont ((float) headerHeight * 0.4f);
    g.drawFittedText (presetUpdateText, headerBounds, Justification::centred, 1);
}

void PresetsSyncDialog::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (headerHeight);
    auto footerBounds = bounds.removeFromBottom (footerHeight);

    presetsList.setBounds (bounds.reduced (5));

    okButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
    cancelButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
}
