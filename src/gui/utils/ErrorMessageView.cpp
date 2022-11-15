#include "ErrorMessageView.h"
#include "gui/BYODPluginEditor.h"

ErrorMessageView::ErrorMessageView()
{
    addAndMakeVisible (title);
    title.setJustificationType (Justification::centred);
    title.setColour (Label::ColourIds::textColourId, Colours::red);

    addAndMakeVisible (message);
    message.setJustificationType (Justification::centred);

    addAndMakeVisible (closeButton);
    closeButton.onClick = [this]
    { setVisible (false); };
}

void ErrorMessageView::showErrorMessage (const String& title, const String& message, const String& buttonText, Component* comp)
{
#if ! JUCE_IOS
    NativeMessageBox::showAsync (MessageBoxOptions()
                                     .withIconType (MessageBoxIconType::WarningIcon)
                                     .withTitle (title)
                                     .withMessage (message)
                                     .withButton (buttonText)
                                     .withAssociatedComponent (comp),
                                 nullptr);
#else
    BYODPluginEditor* topLevelEditor = nullptr;
    while (comp != nullptr)
    {
        if ((topLevelEditor = dynamic_cast<BYODPluginEditor*> (comp)))
            break;
        comp = comp->getParentComponent();
    }

    if (topLevelEditor == nullptr)
    {
        // Unable to find top level component!
        jassertfalse;
        return;
    }

    auto& errorMessageView = topLevelEditor->getErrorMessageView();
    errorMessageView.setParameters (title, message, buttonText);
    errorMessageView.setVisible (true);
#endif
}

void ErrorMessageView::setParameters (const juce::String& titleText,
                                      const juce::String& messageText,
                                      const juce::String& buttonText)
{
    setAlwaysOnTop (true);
    title.setText (titleText, juce::dontSendNotification);
    message.setText (messageText, juce::dontSendNotification);
    closeButton.setButtonText (buttonText);
}

void ErrorMessageView::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.85f));
}

void ErrorMessageView::resized()
{
    const auto titleX = proportionOfWidth (0.1f);
    const auto titleWidth = proportionOfWidth (0.8f);
    const auto titleY = proportionOfHeight (0.05f);
    const auto titleHeight = proportionOfHeight (0.1f);
    title.setBounds (titleX, titleY, titleWidth, titleHeight);
    title.setFont (Font { (float) titleHeight * 0.8f }.boldened());

    const auto messageX = proportionOfWidth (0.1f);
    const auto messageWidth = proportionOfWidth (0.8f);
    const auto messageY = proportionOfHeight (0.2f);
    const auto messageHeight = proportionOfHeight (0.5f);
    message.setBounds (messageX, messageY, messageWidth, messageHeight);
    message.setFont (Font { (float) messageHeight * 0.075f });

    const auto buttonX = proportionOfWidth (0.45f);
    const auto buttonWidth = proportionOfWidth (0.1f);
    const auto buttonY = proportionOfHeight (0.8f);
    const auto buttonHeight = proportionOfHeight (0.1f);
    closeButton.setBounds (buttonX, buttonY, buttonWidth, buttonHeight);
}
