#include "InfoComponent.h"

InfoComponent::InfoComponent()
{
    setOpaque (false);

    xButton.setButtonText ("x");
    xButton.setColour (TextButton::buttonColourId, Colours::transparentWhite);
    xButton.setColour (ComboBox::outlineColourId, Colours::transparentWhite);
    xButton.setColour (TextButton::textColourOffId, Colours::white);
    xButton.onClick = [=]
    { setVisible (false); };
    addAndMakeVisible (xButton);
}

void InfoComponent::paint (Graphics& g)
{
    auto bounds = getLocalBounds();
    auto fBounds = bounds.toFloat();

    g.setColour (Colours::darkgrey.darker());
    g.fillRoundedRectangle (fBounds, 5.0f);

    g.setColour (Colours::slategrey.brighter());
    g.drawRoundedRectangle (fBounds, 5.0f, 5.0f);

    g.setColour (Colours::white);
    g.setFont (Font (28.0f).boldened());
    auto nameBounds = bounds.removeFromTop (50).reduced (5);
    g.drawFittedText (procName, nameBounds, Justification::centred, 1);

    g.setFont (Font (16.0f));
    auto authorsBounds = bounds.removeFromBottom (50).reduced (5);
    g.drawFittedText (authors, authorsBounds, Justification::centred, 5);
    g.drawFittedText (description, bounds.reduced (20, 0), Justification::centred, 10);
}

void InfoComponent::resized()
{
    constexpr int xButtonSize = 27;
    xButton.setBounds (getWidth() - xButtonSize, 0, xButtonSize, xButtonSize);
}

void InfoComponent::setInfoForProc (const String& name, const ProcessorUIOptions::ProcInfo& info)
{
    procName = name;
    description = info.description;

    numAuthors = info.authors.size();
    authors = numAuthors == 1 ? "Author: " : "Authors: ";
    for (int i = 0; i < numAuthors; ++i)
    {
        if (i + 1 == numAuthors)
        {
            authors += info.authors[i];
            continue;
        }

        authors += info.authors[i] + ", ";
    }
}
