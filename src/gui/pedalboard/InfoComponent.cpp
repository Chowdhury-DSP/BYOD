#include "InfoComponent.h"

InfoComponent::InfoComponent()
{
    setOpaque (false);

    xButton.setButtonText ("x");
    xButton.setColour (TextButton::buttonColourId, Colours::transparentWhite);
    xButton.setColour (ComboBox::outlineColourId, Colours::transparentWhite);
    xButton.setColour (TextButton::textColourOffId, Colours::white);
    xButton.onClick = [this]
    {
        setVisible (false);
    };
    addAndMakeVisible (xButton);

    linkButton.setButtonText ("Click for more information...");
    linkButton.setColour (HyperlinkButton::textColourId, Colours::orangered.brighter());
    addAndMakeVisible (linkButton);
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

    if (linkButton.isVisible())
        bounds.removeFromBottom (30);

    g.drawFittedText (description, bounds.reduced (5, 0), Justification::centred, 10);
}

void InfoComponent::resized()
{
    constexpr int xButtonSize = 27;
    xButton.setBounds (getWidth() - xButtonSize, 0, xButtonSize, xButtonSize);

    linkButton.setFont (16.0f, false, Justification::centred);
    linkButton.setBounds (0, getHeight() - 80, getWidth(), 30);
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

    auto url = URL::createWithoutParsing (info.infoLink);
    if (url.isWellFormed())
    {
        linkButton.setURL (url);
        linkButton.setVisible (true);
    }
    else
    {
        linkButton.setVisible (false);
    }
}
