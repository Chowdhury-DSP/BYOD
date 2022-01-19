#include "PresetsLoginDialog.h"

PresetsLoginDialog::PresetsLoginDialog()
{
    setName ("Preset Server Login");

    auto setupEditor = [=] (TextEditor& entryBox, const String& emptyText)
    {
        entryBox.setText (String(), dontSendNotification);
        entryBox.setColour (TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
        entryBox.setColour (TextEditor::ColourIds::textColourId, Colours::white);
        entryBox.setColour (TextEditor::ColourIds::outlineColourId, Colours::white);
        entryBox.setColour (TextEditor::ColourIds::focusedOutlineColourId, Colours::white);
        entryBox.setColour (TextEditor::ColourIds::highlightColourId, Colours::orange.withAlpha (0.5f));
        entryBox.setTextToShowWhenEmpty (emptyText, Colours::white.darker (0.4f));
        addAndMakeVisible (entryBox);
    };

    setupEditor (username, "Username...");
    setupEditor (password, "Password...");

    auto setupButton = [=] (TextButton& button)
    {
        button.setColour (TextButton::ColourIds::buttonColourId, Colours::transparentBlack);
        button.setColour (TextButton::ColourIds::textColourOffId, Colours::white);
        addAndMakeVisible (button);
    };

    setupButton (loginButton);
    setupButton (cancelButton);
    setupButton (registerButton);

    loginButton.onClick = [&]
    {
        userManager->attemptToLogIn (username.getText(), password.getText());
        if (userManager->getIsLoggedIn())
        {
            getParentComponent()->setVisible (false);
            loginChangeCallback();
        }
    };

    cancelButton.onClick = [&]
    { getParentComponent()->setVisible (false); };

    registerButton.onClick = [&]
    {
        userManager->createNewUser (username.getText(), password.getText());
        if (userManager->getIsLoggedIn())
        {
            getParentComponent()->setVisible (false);
            loginChangeCallback();
        }
    };

    setSize (300, 120);
}

void PresetsLoginDialog::visibilityChanged()
{
    username.setText (String(), dontSendNotification);
    password.setText (String(), dontSendNotification);
}

void PresetsLoginDialog::resized()
{
    auto bounds = getLocalBounds();

    username.setBounds (bounds.removeFromTop (proportionOfHeight (0.33f)).reduced (proportionOfWidth (0.1f), 5));
    password.setBounds (bounds.removeFromTop (proportionOfHeight (0.33f)).reduced (proportionOfWidth (0.1f), 5));

    loginButton.setBounds (bounds.removeFromLeft (proportionOfWidth (0.33f)).reduced (5));
    cancelButton.setBounds (bounds.removeFromLeft (proportionOfWidth (0.33f)).reduced (5));
    registerButton.setBounds (bounds.removeFromLeft (proportionOfWidth (0.33f)).reduced (5));
}
