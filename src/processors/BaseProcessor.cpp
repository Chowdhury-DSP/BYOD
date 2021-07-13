#include "BaseProcessor.h"

BaseProcessor::BaseProcessor (const String& name,
                              AudioProcessorValueTreeState::ParameterLayout params,
                              UndoManager* um) : JuceProcWrapper (name),
                                                 vts (*this, um, Identifier ("Parameters"), std::move (params))
{
    onOffParam = vts.getRawParameterValue ("on_off");
    uiOptions.lnf = lnfAllocator->getLookAndFeel<ProcessorLNF>();
}

std::unique_ptr<XmlElement> BaseProcessor::toXML()
{
    auto state = vts.copyState();
    return state.createXml();
}

void BaseProcessor::fromXML (XmlElement* xml)
{
    if (xml != nullptr)
        if (xml->hasTagName (vts.state.getType()))
            vts.state = ValueTree::fromXml (*xml); // don't use `replaceState()` so that the UndoManager doesn't clear
}
