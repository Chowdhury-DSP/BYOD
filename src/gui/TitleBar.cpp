#include "TitleBar.h"
#include "BYOD.h"
#include "GUIConstants.h"

namespace GUIColours = GUIConstants::Colours;

struct BYODInfoProvider : public chowdsp::StandardInfoProvider
{
    //    static constexpr bool showDebugFlag() { return false; }
    static juce::String getWrapperTypeString (const BYOD& proc) { return proc.getWrapperTypeString(); }
};

struct TitleBar::BYODInfoComp : public chowdsp::InfoComp<BYODInfoProvider, BYOD>
{
    BYODInfoComp (BYOD& plugin) : chowdsp::InfoComp<BYODInfoProvider, BYOD> (plugin) {}
};

TitleBar::TitleBar (BYOD& plugin)
{
    addAndMakeVisible (titleComp);
    titleComp.setColour (chowdsp::TitleComp::text1ColourID, GUIColours::titleTextColour);

    infoComp = std::make_unique<BYODInfoComp> (plugin);
    addAndMakeVisible (infoComp.get());
    infoComp->setColour (BYODInfoComp::text1ColourID, GUIColours::infoTextColour);
}

TitleBar::~TitleBar() = default;

void TitleBar::paint (Graphics& g)
{
    g.fillAll (GUIConstants::Colours::barBackgroundShade);
}

void TitleBar::resized()
{
    auto bounds = getLocalBounds().reduced (10, 0);

    const auto titleFont = 0.85f * (float) getHeight();
    titleComp.setStrings ("BYOD", {}, titleFont);
    const auto titleWidth = proportionOfWidth (0.25f);
    titleComp.setBounds (bounds.removeFromLeft (titleWidth));

    const auto infoWidth = jmin (proportionOfWidth (0.5f), 400);
    infoComp->setBounds (bounds.removeFromRight (infoWidth));
}
