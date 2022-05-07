#include "BYODPluginEditor.h"
#include "GUIConstants.h"
#include "utils/LookAndFeels.h"

BYODPluginEditor::BYODPluginEditor (BYOD& p) : AudioProcessorEditor (p),
                                               plugin (p),
                                               titleBar (plugin),
                                               board (plugin.getProcChain()),
                                               toolBar (plugin)
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

    auto& uiState = plugin.getStateManager().getUIState();
    uiState.attachToComponent (*this);
    auto [width, height] = uiState.getLastEditorSize();
    setSize (width, height);
}

void BYODPluginEditor::paint (Graphics& g)
{
    g.fillAll (GUIConstants::Colours::backgroundGrey);
}

void BYODPluginEditor::resized()
{
    auto bounds = getLocalBounds();
    const auto barHeight = jmin (proportionOfHeight (0.06f), 50);

    titleBar.setBounds (bounds.removeFromTop (barHeight));

#if JUCE_IOS
    toolBar.setBounds (bounds.removeFromTop (barHeight));
#else
    toolBar.setBounds (bounds.removeFromBottom (barHeight));
#endif

    board.setBounds (bounds);
}
