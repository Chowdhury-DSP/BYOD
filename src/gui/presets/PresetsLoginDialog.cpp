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
    password.setPasswordCharacter (juce_wchar (L'●'));

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
    { doRegisterLoginAction(); };

    cancelButton.onClick = [&]
    { getParentComponent()->setVisible (false); };

    registerButton.onClick = [&]
    { doRegisterLoginAction (true); };

    addChildComponent (spinner);

    setSize (300, 120);
}

void PresetsLoginDialog::doRegisterLoginAction (bool registerAction)
{
    spinner.setVisible (true);
    jobPool->addJob (
        [registerAction, safeComp = Component::SafePointer (this), usernameText = username.getText(), passwordText = password.getText()]
        {
            if (registerAction)
                safeComp->userManager->createNewUser (usernameText, passwordText);
            else
                safeComp->userManager->attemptToLogIn (usernameText, passwordText);

            if (safeComp == nullptr) // component was deleted while waiting for job to run...
                return;

            if (safeComp->userManager->getIsLoggedIn())
            {
                safeComp->loginChangeCallback();
                PresetsServerJobPool::callSafeOnMessageThread (safeComp, [] (auto& c)
                                                               { c.getParentComponent()->setVisible (false); });
            }

            PresetsServerJobPool::callSafeOnMessageThread (safeComp,
                                                           [] (auto& c)
                                                           {
                                                               c.spinner.setVisible (false);
                                                           });
        });
}

void PresetsLoginDialog::visibilityChanged()
{
    username.setText (String(), dontSendNotification);
    password.setText (String(), dontSendNotification);
}

void PresetsLoginDialog::resized()
{
    auto bounds = getLocalBounds();
    spinner.setBounds (bounds);

    username.setBounds (bounds.removeFromTop (proportionOfHeight (0.33f)).reduced (proportionOfWidth (0.1f), 5));
    password.setBounds (bounds.removeFromTop (proportionOfHeight (0.33f)).reduced (proportionOfWidth (0.1f), 5));

    loginButton.setBounds (bounds.removeFromLeft (proportionOfWidth (0.33f)).reduced (5));
    cancelButton.setBounds (bounds.removeFromLeft (proportionOfWidth (0.33f)).reduced (5));
    registerButton.setBounds (bounds.removeFromLeft (proportionOfWidth (0.33f)).reduced (5));
}
