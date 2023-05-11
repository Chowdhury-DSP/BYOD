#include "NetlistViewer.h"
#include "gui/pedalboard/editors/ProcessorEditor.h"
#include "processors/BaseProcessor.h"

namespace netlist
{
constexpr int rowHeight = 27;
constexpr int schematicPad = 10;

NetlistViewer::NetlistViewer (CircuitQuantityList& quantities)
{
    juce::Component::setName ("Circuit Netlist");

    for (auto [idx, element] : chowdsp::enumerate (quantities))
    {
        auto& [componentLabel, valueLabel] = *labelPairs.add (std::make_unique<std::pair<ComponentLabel, Label>>());

        componentLabel.setText (element.name, juce::dontSendNotification);
        componentLabel.setJustificationType (Justification::centred);
        componentLabel.setColour (Label::textColourId, Colours::black);
        componentLabel.onDoubleClick = [&vl = valueLabel, &el = element]
        {
            el.value = el.defaultValue;
            el.needsUpdate = true;
            vl.setText (toString (el), juce::dontSendNotification);
        };
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
            el.needsUpdate = true;
            vl.setText (toString (el), juce::dontSendNotification);
        };
        addAndMakeVisible (valueLabel);
    }

    schematicSVG = juce::Drawable::createFromImageData (quantities.schematicSVG.data, quantities.schematicSVG.size);
    addAndMakeVisible (schematicSVG.get());

    setSize (300 + schematicSVG->getWidth() + 2 * schematicPad,
             juce::jmax (((int) quantities.size() + 1) * rowHeight,
                         schematicSVG->getHeight() + 2 * schematicPad));
}

void NetlistViewer::paint (Graphics& g)
{
    g.fillAll (Colours::white);

    auto bounds = getLocalBounds();
    bounds.removeFromRight (schematicSVG->getWidth() + 2 * schematicPad);

    g.setColour (Colours::black);
    const auto xCoord = 0.5f * (float) bounds.getWidth();
    g.drawLine (juce::Line<float> { xCoord, 0.0f, xCoord, (float) getHeight() }, 2.0f);
    g.drawLine (juce::Line<float> { (float) bounds.getWidth(), 0.0f, (float) bounds.getWidth(), (float) getHeight() }, 2.0f);

    for (int i = 0; i <= labelPairs.size(); ++i)
    {
        const auto yCoord = float (rowHeight * (i + 1));
        g.drawLine (juce::Line<float> { 0.0f, yCoord, (float) bounds.getWidth(), yCoord }, 2.0f);
    }

    g.setFont (Font { 20.0f }.boldened());
    const auto halfWidth = bounds.proportionOfWidth (0.5f);
    g.drawFittedText ("Component", { 0, 0, halfWidth, rowHeight }, Justification::centred, 1);
    g.drawFittedText ("Value", { halfWidth, 0, halfWidth, rowHeight }, Justification::centred, 1);
}

void NetlistViewer::resized()
{
    auto bounds = getLocalBounds();
    const auto svgBounds = bounds.removeFromRight (schematicSVG->getWidth() + 2 * schematicPad)
                               .removeFromTop (schematicSVG->getHeight() + 2 * schematicPad);
    schematicSVG->setBounds (svgBounds.reduced (schematicPad));

    const auto halfWidth = bounds.proportionOfWidth (0.5f);

    for (auto [i, labelPair] : chowdsp::enumerate (labelPairs))
    {
        auto& [componentLabel, valueLabel] = *labelPair;
        const auto yCoord = rowHeight * int (i + 1);
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
} // namespace netlist
