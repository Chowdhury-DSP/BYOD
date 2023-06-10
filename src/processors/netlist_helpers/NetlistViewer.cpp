#include "NetlistViewer.h"
#include "gui/pedalboard/editors/ProcessorEditor.h"
#include "gui/utils/ErrorMessageView.h"
#include "processors/BaseProcessor.h"

namespace netlist
{
constexpr int rowHeight = 27;
constexpr int schematicPad = 10;
constexpr int maxItemsPerColumn = 16;

void NetlistViewer::ComponentLabel::mouseDoubleClick (const MouseEvent& e)
{
    if (onDoubleClick != nullptr)
    {
        onDoubleClick();
        return;
    }

    Label::mouseDoubleClick (e);
}

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

    if (! quantities.extraNote.empty())
    {
        noteLabel.setText ("Note: " + quantities.extraNote, juce::dontSendNotification);
        noteLabel.setJustificationType (Justification::topLeft);
        noteLabel.setColour (Label::textColourId, Colours::black);
        addAndMakeVisible (noteLabel);
    }

    schematicSVG = juce::Drawable::createFromImageData (quantities.schematicSVG.data, quantities.schematicSVG.size);
    addAndMakeVisible (schematicSVG.get());

    if (needsTwoColumns())
    {
        const auto nextEvenNumber = [] (int n)
        {
            return n + (n % 2);
        };

        setSize (450 + schematicSVG->getWidth() + 2 * schematicPad,
                 juce::jmax ((maxItemsPerColumn + 1) * rowHeight,
                             schematicSVG->getHeight() + 2 * schematicPad));
    }
    else
    {
        setSize (250 + schematicSVG->getWidth() + 2 * schematicPad,
                 juce::jmax (((int) quantities.size() + 1) * rowHeight,
                             schematicSVG->getHeight() + 2 * schematicPad));
    }

    pluginSettings->addProperties ({ { netlistWarningShownID, false } });
}

bool NetlistViewer::needsTwoColumns() const
{
    return labelPairs.size() > maxItemsPerColumn;
}

bool NetlistViewer::showWarningView()
{
    if (! pluginSettings->getProperty<bool> (netlistWarningShownID))
    {
        static constexpr std::string_view netlistViewWarning = "The netlist view allows you to customise the internal parameters "
                                                               "of the circuit model. While we have attempted to constrain the "
                                                               "parameters to a reasonably safe range, please take care when editing "
                                                               "the parameters, by using a safety limiter and avoiding the use of "
                                                               "headphones.\n\n"
                                                               "Are you sure you want to continue?";

        if (ErrorMessageView::showYesNoBox ("Warning",
                                            chowdsp::toString (netlistViewWarning),
                                            this))
        {
            pluginSettings->setProperty (netlistWarningShownID, true);
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

void NetlistViewer::paint (Graphics& g)
{
    g.fillAll (Colours::white);

    auto bounds = getLocalBounds();
    bounds.removeFromRight (schematicSVG->getWidth() + 2 * schematicPad);

    g.setColour (Colours::black);
    g.setFont (Font { 20.0f }.boldened());
    if (needsTwoColumns())
    {
        const auto halfWidth = bounds.proportionOfWidth (0.5f);
        for (int i = 0; i < maxItemsPerColumn; ++i)
        {
            const auto yCoord = float (rowHeight * (i + 1));
            g.drawLine (juce::Line<float> { 0.0f, yCoord, (float) halfWidth, yCoord }, 2.0f);
        }
        for (int i = maxItemsPerColumn; i <= labelPairs.size(); ++i)
        {
            const auto yCoord = float (rowHeight * (i - maxItemsPerColumn + 1));
            g.drawLine (juce::Line<float> { (float) halfWidth, yCoord, (float) bounds.getWidth(), yCoord }, 2.0f);
        }

        const auto xCoord = 0.25f * (float) bounds.getWidth();
        const auto length = noteLabel.isVisible() ? float (rowHeight * (labelPairs.size() - maxItemsPerColumn + 1)) : (float) getHeight();
        g.drawLine (juce::Line<float> { xCoord, 0.0f, xCoord, (float) getHeight() }, 2.0f);
        g.drawLine (juce::Line<float> { 2.0f * xCoord, 0.0f, 2.0f * xCoord, (float) getHeight() }, 2.0f);
        g.drawLine (juce::Line<float> { 3.0f * xCoord, 0.0f, 3.0f * xCoord, length }, 2.0f);
        g.drawLine (juce::Line<float> { (float) bounds.getWidth(), 0.0f, (float) bounds.getWidth(), (float) getHeight() }, 2.0f);

        const auto quarterWidth = bounds.proportionOfWidth (0.25f);
        g.drawFittedText ("Component", { 0, 0, quarterWidth, rowHeight }, Justification::centred, 1);
        g.drawFittedText ("Value", { quarterWidth, 0, quarterWidth, rowHeight }, Justification::centred, 1);
        g.drawFittedText ("Component", { 2 * quarterWidth, 0, quarterWidth, rowHeight }, Justification::centred, 1);
        g.drawFittedText ("Value", { 3 * quarterWidth, 0, quarterWidth, rowHeight }, Justification::centred, 1);
    }
    else
    {
        for (int i = 0; i <= labelPairs.size(); ++i)
        {
            const auto yCoord = float (rowHeight * (i + 1));
            g.drawLine (juce::Line<float> { 0.0f, yCoord, (float) bounds.getWidth(), yCoord }, 2.0f);
        }

        const auto xCoord = 0.5f * (float) bounds.getWidth();
        const auto length = noteLabel.isVisible() ? float (rowHeight * (labelPairs.size() + 1)) : (float) getHeight();
        g.drawLine (juce::Line<float> { xCoord, 0.0f, xCoord, length }, 2.0f);
        g.drawLine (juce::Line<float> { (float) bounds.getWidth(), 0.0f, (float) bounds.getWidth(), (float) getHeight() }, 2.0f);

        const auto halfWidth = bounds.proportionOfWidth (0.5f);
        g.drawFittedText ("Component", { 0, 0, halfWidth, rowHeight }, Justification::centred, 1);
        g.drawFittedText ("Value", { halfWidth, 0, halfWidth, rowHeight }, Justification::centred, 1);
    }
}

void NetlistViewer::resized()
{
    auto bounds = getLocalBounds();
    const auto svgBounds = bounds.removeFromRight (schematicSVG->getWidth() + 2 * schematicPad)
                               .removeFromTop (schematicSVG->getHeight() + 2 * schematicPad);
    schematicSVG->setBounds (svgBounds.reduced (schematicPad));

    if (needsTwoColumns())
    {
        const auto quarterWidth = bounds.proportionOfWidth (0.25f);
        for (int i = 0; i < maxItemsPerColumn; ++i)
        {
            auto& [componentLabel, valueLabel] = *labelPairs[i];
            const auto yCoord = rowHeight * int (i + 1);
            componentLabel.setBounds (0, yCoord, quarterWidth, rowHeight);
            valueLabel.setBounds (quarterWidth, yCoord, quarterWidth, rowHeight);
        }
        for (int i = maxItemsPerColumn; i < labelPairs.size(); ++i)
        {
            auto& [componentLabel, valueLabel] = *labelPairs[i];
            const auto yCoord = rowHeight * int (i - maxItemsPerColumn + 1);
            componentLabel.setBounds (2 * quarterWidth, yCoord, quarterWidth, rowHeight);
            valueLabel.setBounds (3 * quarterWidth, yCoord, quarterWidth, rowHeight);
        }
    }
    else
    {
        const auto halfWidth = bounds.proportionOfWidth (0.5f);
        for (auto [i, labelPair] : chowdsp::enumerate (labelPairs))
        {
            auto& [componentLabel, valueLabel] = *labelPair;
            const auto yCoord = rowHeight * int (i + 1);
            componentLabel.setBounds (0, yCoord, halfWidth, rowHeight);
            valueLabel.setBounds (halfWidth, yCoord, halfWidth, rowHeight);
        }
    }

    if (noteLabel.isVisible())
    {
        const auto width = 2 * labelPairs.getLast()->second.getWidth();
        const auto xPos = labelPairs.getLast()->second.getX() - width / 2;
        const auto yPos = labelPairs.getLast()->second.getBottom();
        noteLabel.setBounds (xPos, yPos, width, getHeight() - yPos);
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
        if (netlistWindow->getViewComponent().showWarningView())
        {
            netlistWindowCast->show();
            processor.setNetlistWindow (std::move (netlistWindow));
        }
    };
    return item;
}
} // namespace netlist
