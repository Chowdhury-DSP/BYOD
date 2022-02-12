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
    explicit PresetsListModel (const PresetManager::PresetUpdateList& presetsToUpdate)
    {
        for (const auto& [preset, updateType] : presetsToUpdate)
            updateInfo.add (PresetUpdateInfo { preset.getName(), updateType });
    }

    int getNumRows() override
    {
        return updateInfo.size();
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool /*rowIsSelected*/) override
    {
        auto bounds = Rectangle { width, height };

        g.setColour (Colours::white.withAlpha (0.4f));
        g.drawLine (Line { bounds.getTopLeft(), bounds.getTopRight() }.toFloat(), 1.0f);
        if (rowNumber >= updateInfo.size() - 1)
            g.drawLine (Line { bounds.getBottomLeft(), bounds.getBottomRight() }.toFloat(), 1.0f);

        g.setFont ((float) height * 0.6f);
        g.setColour (Colours::white);
        auto name = updateInfo[rowNumber].name;
        auto nameBounds = bounds.removeFromLeft (width * 2 / 3);
        g.drawFittedText (name, nameBounds, Justification::centred, 1);

        auto updateType = updateInfo[rowNumber].updateType;
        g.setColour (updateType == PresetManager::Adding ? Colours::lightgreen : Colours::yellow);
        auto typeStr = "(" + String (magic_enum::enum_name (updateType).data()) + ")";
        g.drawFittedText (typeStr, bounds, Justification::centred, 1);
    }

    struct PresetUpdateInfo
    {
        String name;
        PresetManager::PresetUpdate updateType;
    };

    Array<PresetUpdateInfo> updateInfo;
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

    setSize (noUpdateWidth, noUpdateHeight);
}

PresetsSyncDialog::~PresetsSyncDialog()
{
    presetUpdateList = nullptr;
}

void PresetsSyncDialog::updatePresetsList (PresetManager::PresetUpdateList& presetsToUpdate)
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

    presetUpdateList = &presetsToUpdate;
    okButton.onClick = [&]
    {
        if (presetUpdateList != nullptr)
        {
            runUpdateCallback (*presetUpdateList);
            getParentComponent()->setVisible (false);
            presetUpdateList = nullptr;
        }
    };
}

void PresetsSyncDialog::paint (Graphics& g)
{
    g.fillAll (Colours::black);

    auto headerBounds = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (Colours::white);
    g.setFont ((float) headerHeight * 0.5f);
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
