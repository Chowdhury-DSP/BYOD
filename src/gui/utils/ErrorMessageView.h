#pragma once

#include <pch.h>

class ErrorMessageView : public Component
{
public:
    ErrorMessageView();

    static void showErrorMessage (const String& title, const String& message, const String& buttonText, Component* comp);

    void setParameters (const String& title, const String& message, const String& buttonText);

    void paint (Graphics& g) override;
    void resized() override;

private:
    Label title;
    Label message;
    TextButton closeButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ErrorMessageView)
};
