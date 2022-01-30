#include "PresetsSaveDialog.h"

namespace
{
constexpr int headerHeight = 40;
constexpr int footerHeight = 50;
constexpr int labelsWidth = 80;
constexpr int itemHeight = 50;

struct CustomLabel : Label
{
    TextEditor* createEditorComponent() override
    {
        auto* editor = Label::createEditorComponent();
        editor->setJustification (Justification::centred);
        editor->setMultiLine (false);

        return editor;
    }
};
} // namespace

PresetsSaveDialog::PresetsSaveDialog()
{
    auto setupLabel = [&] (auto& label)
    {
        label = std::make_unique<CustomLabel>();
        label->setColour (Label::backgroundColourId, Colours::transparentBlack);
        label->setColour (Label::outlineColourId, Colours::white);

        label->setJustificationType (Justification::centred);
        label->setEditable (true);

        addAndMakeVisible (label.get());
    };

    setupLabel (nameLabel);
    setupLabel (categoryLabel);

    addAndMakeVisible (publicSwitch);
    publicSwitch.setEnabled (false); // @TODO: temporary, until this works...

    addAndMakeVisible (okButton);
    okButton.onClick = [&]
    {
        const auto nameText = nameLabel->getText (true);
        const auto categoryText = categoryLabel->getText (true);
        const auto isPublic = publicSwitch.getToggleState();

        if (nameText.isEmpty())
        {
            NativeMessageBox::showMessageBox (MessageBoxIconType::WarningIcon, "Preset Save Error!", "Preset name must not be empty");
            return;
        }

        presetSaveCallback (nameText, categoryText, isPublic);

        getParentComponent()->setVisible (false);
    };

    addAndMakeVisible (cancelButton);
    cancelButton.onClick = [&]
    { getParentComponent()->setVisible (false); };

    setSize (400, 240);
}

void PresetsSaveDialog::prepareToShow (bool shouldSave)
{
    isSaveMode = shouldSave;
    getParentComponent()->setName (isSaveMode ? "Preset Saving" : "Preset Editing");

    nameLabel->setText ("MyPreset", dontSendNotification);
    categoryLabel->setText ("", dontSendNotification);
    publicSwitch.setToggleState (false, dontSendNotification);
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

    drawItemLabel (*nameLabel, "Name: ");
    drawItemLabel (*categoryLabel, "Category: ");
}

void PresetsSaveDialog::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (headerHeight);
    auto footerBounds = bounds.removeFromBottom (footerHeight);

    bounds.removeFromLeft (labelsWidth);
    nameLabel->setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5));
    categoryLabel->setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5));
    publicSwitch.setBounds (bounds.removeFromTop (itemHeight).reduced (10, 5).withWidth (100));

    okButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
    cancelButton.setBounds (footerBounds.removeFromLeft (proportionOfWidth (0.5f)).reduced (5));
}
