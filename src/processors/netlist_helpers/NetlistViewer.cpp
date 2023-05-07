#include "NetlistViewer.h"
#include "processors/BaseProcessor.h"
#include "gui/pedalboard/editors/ProcessorEditor.h"

namespace netlist
{
constexpr int rowHeight = 27;
NetlistViewer::NetlistViewer (CircuitQuantityList& quantities)
{
    juce::Component::setName ("Circuit Netlist");

    for (auto [idx, element] : chowdsp::enumerate (quantities))
    {
        auto& [componentLabel, valueLabel] = *labelPairs.add (std::make_unique<std::pair<Label, Label>>());

        componentLabel.setText (element.name, juce::dontSendNotification);
        componentLabel.setJustificationType (Justification::centred);
        componentLabel.setColour (Label::textColourId, Colours::black);
        addAndMakeVisible (componentLabel);

        valueLabel.setText (toString (element), juce::dontSendNotification);
        valueLabel.setJustificationType (Justification::centred);
        valueLabel.setColour (Label::textColourId, Colours::black);
        valueLabel.setColour (Label::textWhenEditingColourId, Colours::black);
        valueLabel.setColour (TextEditor::highlightColourId, Colours::black.withAlpha (0.2f));
        valueLabel.setColour (TextEditor::highlightedTextColourId, Colours::black);
        valueLabel.setColour (CaretComponent::caretColourId, Colours::black);
        valueLabel.setEditable (true);
        valueLabel.onEditorShow = [&l = valueLabel]
        {
            if (auto* ed = l.getCurrentTextEditor())
                ed->setJustification (Justification::centred);
        };
        valueLabel.onTextChange = [&vl = valueLabel, &el = element]
        {
            el.value = netlist::fromString (vl.getText(), el);
            vl.setText (toString (el), juce::dontSendNotification);
        };
        addAndMakeVisible (valueLabel);
    }

    setSize (300, ((int) quantities.size() + 1) * rowHeight);
}

void NetlistViewer::paint (Graphics& g)
{
    g.fillAll (Colours::white);

    g.setColour (Colours::black);
    const auto xCoord = 0.5f * (float) getWidth();
    g.drawLine (juce::Line<float> { xCoord, 0.0f, xCoord, (float) getHeight() }, 2.0f);

    for (int i = 1; i <= labelPairs.size(); ++i)
    {
        const auto yCoord = (float) getHeight() * (float) i / float (labelPairs.size() + 1);
        g.drawLine (juce::Line<float> { 0.0f, yCoord, (float) getWidth(), yCoord }, 2.0f);
    }

    g.setFont (Font { 20.0f }.boldened());
    const auto halfWidth = proportionOfWidth (0.5f);
    g.drawFittedText ("Component", { 0, 0, halfWidth, rowHeight }, Justification::centred, 1);
    g.drawFittedText ("Value", { halfWidth, 0, halfWidth, rowHeight }, Justification::centred, 1);
}

void NetlistViewer::resized()
{
    const auto halfWidth = proportionOfWidth (0.5f);

    for (auto [i, labelPair] : chowdsp::enumerate (labelPairs))
    {
        auto& [componentLabel, valueLabel] = *labelPair;
        const auto yCoord = roundToInt ((float) getHeight() * (float) (i + 1) / float (labelPairs.size() + 1));
        componentLabel.setBounds (0, yCoord, halfWidth, rowHeight);
        valueLabel.setBounds (halfWidth, yCoord, halfWidth, rowHeight);
    }
}

juce::PopupMenu::Item createNetlistViewerPopupMenuItem (BaseProcessor& processor)
{
    PopupMenu::Item item;
    item.itemID = 99091;
    item.text = "Show netlist";
    item.action = [&processor]
    {
        juce::Logger::writeToLog ("Showing netlist for module: " + processor.getName());

        if (processor.getEditor() == nullptr)
            return;

        auto* topLevelEditor = [&processor]() -> AudioProcessorEditor*
        {
            Component* tlEditor = processor.getEditor()->getParentComponent();
            while (tlEditor != nullptr)
            {
                if (auto* editorCast = dynamic_cast<AudioProcessorEditor*> (tlEditor))
                    return editorCast;

                tlEditor = tlEditor->getParentComponent();
            }
            return nullptr;
        }();

        if (topLevelEditor == nullptr)
            return;

        using NetlistWindow = chowdsp::WindowInPlugin<netlist::NetlistViewer>;
        auto netlistWindow = std::make_unique<NetlistWindow> (*topLevelEditor, *processor.getNetlistCircuitQuantities());
        auto* netlistWindowCast = static_cast<NetlistWindow*> (netlistWindow.get()); // NOLINT
        netlistWindowCast->setResizable (false, false);
        netlistWindowCast->show();
        processor.setNetlistWindow (std::move (netlistWindow));
    };
    return item;
}
}
