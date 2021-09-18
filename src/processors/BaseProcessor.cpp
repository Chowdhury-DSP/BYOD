#include "BaseProcessor.h"

BaseProcessor::BaseProcessor (const String& name,
                              AudioProcessorValueTreeState::ParameterLayout params,
                              UndoManager* um,
                              int nInputs,
                              int nOutputs) : JuceProcWrapper (name),
                                              vts (*this, um, Identifier ("Parameters"), std::move (params)),
                                              numInputs (nInputs),
                                              numOutputs (nOutputs)
{
    onOffParam = vts.getRawParameterValue ("on_off");

    outputProcessors.resize (numOutputs);
    bufferArray.resize (numOutputs);

    uiOptions.lnf = lnfAllocator->getLookAndFeel<ProcessorLNF>();
}

std::unique_ptr<XmlElement> BaseProcessor::toXML()
{
    auto state = vts.copyState();
    auto xml = state.createXml();

    xml->setAttribute ("x_pos", (double) editorPosition.x);
    xml->setAttribute ("y_pos", (double) editorPosition.y);

    return std::move (xml);
}

void BaseProcessor::fromXML (XmlElement* xml)
{
    if (xml != nullptr)
    {
        if (xml->hasTagName (vts.state.getType()))
        {
            vts.state = ValueTree::fromXml (*xml); // don't use `replaceState()` otherwise UndoManager will clear

            auto xPos = (float) xml->getDoubleAttribute ("x_pos");
            auto yPos = (float) xml->getDoubleAttribute ("y_pos");
            editorPosition = Point { xPos, yPos };
        }
    }
}

void BaseProcessor::setPosition (Point<int> pos, Rectangle<int> parentBounds)
{
    editorPosition = pos.toFloat() / Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() };
}

Point<int> BaseProcessor::getPosition (Rectangle<int> parentBounds)
{
    return (editorPosition * Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() }).toInt();
}
