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

    addAndMakeVisible (yesButton);
    yesButton.onClick = [this]
    {
        result = 1;
        setVisible (false);
    };

    addAndMakeVisible (noButton);
    noButton.onClick = [this]
    {
        result = 0;
        setVisible (false);
    };
}

BYODPluginEditor* findTopLevelEditor (Component* currentComponent)
{
    BYODPluginEditor* topLevelEditor = nullptr;
    while (currentComponent != nullptr)
    {
        if ((topLevelEditor = dynamic_cast<BYODPluginEditor*> (currentComponent)))
            break;
        currentComponent = currentComponent->getParentComponent();
    }

    // Unable to find top level component!
    jassert (topLevelEditor != nullptr);

    return topLevelEditor;
}

void ErrorMessageView::showErrorMessage (const String& title, const String& message, const String& buttonText, Component* comp)
{
    // similar to:
//    NativeMessageBox::showAsync (MessageBoxOptions()
//                                     .withIconType (MessageBoxIconType::WarningIcon)
//                                     .withTitle (title)
//                                     .withMessage (message)
//                                     .withButton (buttonText)
//                                     .withAssociatedComponent (comp),
//                                 nullptr);

    if (auto* topLevelEditor = findTopLevelEditor (comp))
    {
        auto& errorMessageView = topLevelEditor->getErrorMessageView();
        errorMessageView.setParameters (title, message, buttonText);
        errorMessageView.setVisible (true);
    }
}

bool ErrorMessageView::showYesNoBox (const juce::String& title, const juce::String& message, juce::Component* comp)
{
    // similar to:
//    return NativeMessageBox::showYesNoBox (MessageBoxIconType::WarningIcon, title, message, comp) == 1;

    if (auto* topLevelEditor = findTopLevelEditor (comp))
    {
        auto& errorMessageView = topLevelEditor->getErrorMessageView();
        errorMessageView.setParametersYesNo (title, message);
        errorMessageView.setVisible (true);

        while (errorMessageView.result < 0)
            MessageManager::getInstance()->runDispatchLoopUntil (50);

        return errorMessageView.result > 0;
    }

    return false;
}

void ErrorMessageView::setParameters (const juce::String& titleText,
                                      const juce::String& messageText,
                                      const juce::String& buttonText)
{
    setAlwaysOnTop (true);
    title.setText (titleText, juce::dontSendNotification);
    message.setText (messageText, juce::dontSendNotification);
    closeButton.setButtonText (buttonText);

    closeButton.setVisible (true);
    yesButton.setVisible (false);
    noButton.setVisible (false);
}

void ErrorMessageView::setParametersYesNo (const String& titleText, const String& messageText)
{
    result = -1;
    setAlwaysOnTop (true);
    title.setText (titleText, juce::dontSendNotification);
    message.setText (messageText, juce::dontSendNotification);

    closeButton.setVisible (false);
    yesButton.setVisible (true);
    noButton.setVisible (true);
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

    const auto yesX = proportionOfWidth (0.375f);
    yesButton.setBounds (yesX, buttonY, buttonWidth, buttonHeight);

    const auto noX = proportionOfWidth (0.525f);
    noButton.setBounds (noX, buttonY, buttonWidth, buttonHeight);
}
