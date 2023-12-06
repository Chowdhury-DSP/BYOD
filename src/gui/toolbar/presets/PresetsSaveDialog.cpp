#include "PresetsSaveDialog.h"
#include "gui/utils/ErrorMessageView.h"
#include "state/presets/PresetInfoHelpers.h"

namespace PresetSaveDims
{
constexpr int headerHeight = 40;
constexpr int footerHeight = 50;
constexpr int labelsWidth = 80;
constexpr int itemHeight = 50;
} // namespace PresetSaveDims

PresetsSaveDialog::PresetsSaveDialog()
{
    auto setupLabel = [&] (auto& label)
    {
        label.setColour (Label::backgroundColourId, Colours::transparentBlack);
        label.setColour (Label::outlineColourId, Colours::white);

        label.setJustificationType (Justification::centred);
        label.setEditable (true);

        addAndMakeVisible (label);
    };

    setupLabel (nameLabel);
    setupLabel (categoryLabel);

    addAndMakeVisible (publicSwitch);

    addAndMakeVisible (okButton);

    addAndMakeVisible (cancelButton);
    cancelButton.onClick = [&]
    { getParentComponent()->setVisible (false); };

#if BYOD_BUILD_PRESET_SERVER
    setSize (400, 240);
#else
    setSize (400, 190);
#endif
}

void PresetsSaveDialog::prepareToShow (const chowdsp::Preset* presetToEdit, const File& presetFile)
{
    isSaveMode = presetToEdit == nullptr;
    getParentComponent()->setName (isSaveMode ? "Preset Saving" : "Preset Editing");

    if (isSaveMode)
    {
        nameLabel.setText ("MyPreset", dontSendNotification);
        categoryLabel.setText ("", dontSendNotification);
        publicSwitch.setToggleState (false, dontSendNotification);
        presetID = {};
    }
    else
    {
        nameLabel.setText (presetToEdit->getName(), dontSendNotification);
        categoryLabel.setText (presetToEdit->getCategory(), dontSendNotification);

#if BYOD_BUILD_PRESET_SERVER
        publicSwitch.setToggleState (PresetInfoHelpers::getIsPublic (*presetToEdit), dontSendNotification);
        presetID = PresetInfoHelpers::getPresetID (*presetToEdit);
#else
        publicSwitch.setToggleState (false, dontSendNotification);
        presetID = {};
#endif
    }

    okButton.onClick = [&, presetFileToDelete = presetFile]
    {
        const auto nameText = nameLabel.getText (true);
        if (nameText.isEmpty())
        {
            ErrorMessageView::showErrorMessage ("Preset Save Error!", "Preset name must not be empty", "OK", getParentComponent());
            return;
        }

        if (! isSaveMode && presetFileToDelete != File())
            presetFileToDelete.deleteFile();

        presetSaveCallback ({ nameText, categoryLabel.getText (true), publicSwitch.getToggleState(), presetID });
        getParentComponent()->setVisible (false);
    };
}

void PresetsSaveDialog::paint (Graphics& g)
{
    g.fillAll (Colours::black);

    g.setColour (Colours::white);
    g.setFont ((float) PresetSaveDims::headerHeight * 0.5f);
    const auto headerBounds = getLocalBounds().removeFromTop (PresetSaveDims::headerHeight);
    const auto headerText = isSaveMode ? "Save Preset:" : "Edit Preset";
    g.drawFittedText (headerText, headerBounds, Justification::centred, 1);

    g.setFont ((float) PresetSaveDims::itemHeight * 0.35f);
    auto drawItemLabel = [&] (const Component& comp, const String& label)
    {
        auto textBounds = Rectangle { PresetSaveDims::labelsWidth, comp.getHeight() }.withY (comp.getY());
        g.drawFittedText (label, textBounds, Justification::centredRight, 1);
    };

    drawItemLabel (nameLabel, "Name: ");
    drawItemLabel (categoryLabel, "Category: ");
}

void PresetsSaveDialog::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (PresetSaveDims::headerHeight);
    auto footerBounds = bounds.removeFromBottom (PresetSaveDims::footerHeight);

    bounds.removeFromLeft (PresetSaveDims::labelsWidth);
    nameLabel.setBounds (bounds.removeFromTop (PresetSaveDims::itemHeight).reduced (10, 5));
    categoryLabel.setBounds (bounds.removeFromTop (PresetSaveDims::itemHeight).reduced (10, 5));

#if BYOD_BUILD_PRESET_SERVER
    publicSwitch.setBounds (bounds.removeFromTop (PresetSaveDims::itemHeight).reduced (10, 5).withWidth (100));
#endif

    okButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
    cancelButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
}
