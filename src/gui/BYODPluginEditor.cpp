#include "BYODPluginEditor.h"
#include "GUIConstants.h"
#include "utils/LookAndFeels.h"

BYODPluginEditor::BYODPluginEditor (BYOD& p) : AudioProcessorEditor (p),
                                               plugin (p),
                                               hostContextProvider (plugin, this),
                                               titleBar (plugin),
                                               board (plugin.getProcChain(), hostContextProvider),
                                               toolBar (plugin, hostContextProvider)
{
    setLookAndFeel (lnfAllocator->addLookAndFeel<ByodLNF>());

    addAndMakeVisible (titleBar);
    addAndMakeVisible (toolBar);
    addAndMakeVisible (board);

    setResizeBehaviour();
}

BYODPluginEditor::~BYODPluginEditor() = default;

void BYODPluginEditor::setResizeBehaviour()
{
    using namespace GUIConstants;
    constrainer.setSizeLimits (int ((float) defaultWidth * 0.5f),
                               int ((float) defaultHeight * 0.5f),
                               int ((float) defaultWidth * 5.0f),
                               int ((float) defaultHeight * 5.0f));
    setConstrainer (&constrainer);
    setResizable (true, true);
}

void BYODPluginEditor::paint (Graphics& g)
{
    g.fillAll (GUIConstants::Colours::backgroundGrey);
}

void BYODPluginEditor::resized()
{
    auto bounds = getLocalBounds();
    const auto barHeight = jlimit (35, 50, proportionOfHeight (0.05f));

    titleBar.setBounds (bounds.removeFromTop (barHeight));

#if JUCE_IOS
    toolBar.setBounds (bounds.removeFromTop (barHeight));
#else
    toolBar.setBounds (bounds.removeFromBottom (barHeight));
#endif

    board.setBounds (bounds);
}

int BYODPluginEditor::getControlParameterIndex (Component& comp)
{
    return hostContextProvider.getParameterIndexForComponent (comp);
}