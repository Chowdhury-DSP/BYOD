#include "BaseProcessor.h"

BaseProcessor::BaseProcessor (const String& name,
                              AudioProcessorValueTreeState::ParameterLayout params,
                              UndoManager* um) : JuceProcWrapper (name),
                                                  vts (*this, um, Identifier ("Parameters"), std::move (params))
{
}

std::unique_ptr<XmlElement> BaseProcessor::toXML()
{
    // @TODO
    return std::make_unique<XmlElement> (String());
}

void BaseProcessor::fromXML (XmlElement* xml)
{
    // @TODO
}
