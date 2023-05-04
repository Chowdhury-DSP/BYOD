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

    addAndMakeVisible (custom1Button);
    addAndMakeVisible (custom2Button);
}

BYODPluginEditor* findTopLevelEditor (Component* currentComponent)
{
    BYODPluginEditor* topLevelEditor = nullptr;
    while (currentComponent != nullptr)
    {
        topLevelEditor = dynamic_cast<BYODPluginEditor*> (currentComponent);
        if (topLevelEditor != nullptr)
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

    juce::Logger::writeToLog ("ERROR MESSAGE REPORTED: " + title + "(" + message + ")");

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

void ErrorMessageView::showCrashLogView (File logFile, Component* comp) // NOLINT
{
    ErrorMessageView::showCustomMessage (
        "Crash detected!",
        "A previous instance of this plugin has crashed! Would you like to view the logs?",
#if JUCE_IOS
        "Copy Logs",
        [logFile]
        {
            SystemClipboard::copyTextToClipboard (logFile.loadFileAsString());
        },
#else
        "Show Log File",
        [logFile]
        {
            logFile.startAsProcess();
        },
#endif
        "Cancel",
        [] {},
        comp);
}

void ErrorMessageView::showCustomMessage (const String& titleText,
                                          const String& messageText,
                                          const String& button1Text,
                                          std::function<void()>&& button1Action,
                                          const String& button2Text,
                                          std::function<void()>&& button2Action,
                                          Component* comp)
{
    if (auto* topLevelEditor = findTopLevelEditor (comp))
    {
        auto& errorMessageView = topLevelEditor->getErrorMessageView();
        errorMessageView.setParametersCustom (titleText, messageText, button1Text, std::move (button1Action), button2Text, std::move (button2Action));
        errorMessageView.setVisible (true);
    }
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
    custom1Button.setVisible (false);
    custom2Button.setVisible (false);
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
    custom1Button.setVisible (false);
    custom2Button.setVisible (false);
}

void ErrorMessageView::setParametersCustom (const String& titleText,
                                            const String& messageText,
                                            const String& button1Text,
                                            std::function<void()>&& button1Action,
                                            const String& button2Text,
                                            std::function<void()>&& button2Action)
{
    result = -1;
    setAlwaysOnTop (true);
    title.setText (titleText, juce::dontSendNotification);
    message.setText (messageText, juce::dontSendNotification);

    closeButton.setVisible (false);
    yesButton.setVisible (false);
    noButton.setVisible (false);
    custom1Button.setVisible (true);
    custom2Button.setVisible (true);

    custom1Button.setButtonText (button1Text);
    custom1Button.onClick = [this, func = std::move (button1Action)]
    {
        func();
        setVisible (false);
    };

    custom2Button.setButtonText (button2Text);
    custom2Button.onClick = [this, func = std::move (button2Action)]
    {
        func();
        setVisible (false);
    };
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

    custom1Button.setBounds (yesButton.getBoundsInParent());
    custom2Button.setBounds (noButton.getBoundsInParent());
}
