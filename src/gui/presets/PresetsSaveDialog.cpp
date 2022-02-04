#include "PresetsSaveDialog.h"
#include "state/presets/PresetManager.h"

namespace
{
constexpr int headerHeight = 40;
constexpr int footerHeight = 50;
constexpr int labelsWidth = 80;
constexpr int itemHeight = 50;
} // namespace

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

    setSize (400, 240);
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
    }
    else
    {
        nameLabel.setText (presetToEdit->getName(), dontSendNotification);
        categoryLabel.setText (presetToEdit->getCategory(), dontSendNotification);
        publicSwitch.setToggleState (presetToEdit->extraInfo.getBoolAttribute (PresetManager::isPublicTag), dontSendNotification);
    }

    okButton.onClick = [&, presetFileToDelete = presetFile]
    {
        const auto nameText = nameLabel.getText (true);
        if (nameText.isEmpty())
        {
            NativeMessageBox::showMessageBox (MessageBoxIconType::WarningIcon, "Preset Save Error!", "Preset name must not be empty");
            return;
        }

        if (! isSaveMode && presetFileToDelete != File())
            presetFileToDelete.deleteFile();

        presetSaveCallback (nameText, categoryLabel.getText (true), publicSwitch.getToggleState());
        getParentComponent()->setVisible (false);
    };
}

void PresetsSaveDialog::paint (Graphics& g)
{
    g.fillAll (Colours::black);

    g.setColour (Colours::white);
    g.setFont ((float) headerHeight * 0.5f);
    const auto headerBounds = getLocalBounds().removeFromTop (headerHeight);
    const auto headerText = isSaveMode ? "Save Preset:" : "Edit Preset";
    g.drawFittedText (headerText, headerBounds, Justification::centred, 1);

    g.setFont ((float) itemHeight * 0.35f);
    auto drawItemLabel = [&] (const Component& comp, const String& label)
    {
        auto textBounds = Rectangle { labelsWidth, comp.getHeight() }.withY (comp.getY());
        g.drawFittedText (label, textBounds, Justification::centredRight, 1);
    };

    drawItemLabel (nameLabel, "Name: ");
    drawItemLabel (categoryLabel, "Category: ");
}

void PresetsSaveDialog::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (headerHeight);
    auto footerBounds = bounds.removeFromBottom (footerHeight);

    bounds.removeFromLeft (labelsWidth);
    nameLabel.setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5));
    categoryLabel.setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5));
    publicSwitch.setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5).withWidth (100));

    okButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
    cancelButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
}
