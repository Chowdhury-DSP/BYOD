#include "AmpIRs.h"
#include "gui/utils/ErrorMessageView.h"

void AmpIRs::loadIRFromStream (std::unique_ptr<InputStream>&& stream, const String& name, const juce::File& file, Component* associatedComp)
{
    auto failToLoad = [this, associatedComp] (const String& message)
    {
        ErrorMessageView::showErrorMessage ("Unable to load IR!", message, "OK", associatedComp);
        vts.getParameter (irTag)->setValueNotifyingHost (0.0f);
    };

    if (stream == nullptr)
    {
        failToLoad ("The following IR file was not valid: " + file.getFullPathName() + " (invalid stream)");
        return;
    }

    MemoryBlock irData;
    const auto originalStreamPosition = stream->getPosition();
    stream->readIntoMemoryBlock (irData);
    stream->setPosition (originalStreamPosition);
    std::unique_ptr<AudioFormatReader> formatReader (audioFormatManager.createReaderFor (std::move (stream)));

    if (formatReader == nullptr)
    {
        failToLoad ("The following IR file was not valid: " + file.getFullPathName() + " (invalid format)");
        return;
    }

    const auto fileLength = static_cast<size_t> (formatReader->lengthInSamples);
    AudioBuffer<float> resultBuffer { jlimit (1, 2, static_cast<int> (formatReader->numChannels)), static_cast<int> (fileLength) };

    if (! formatReader->read (resultBuffer.getArrayOfWritePointers(),
                              resultBuffer.getNumChannels(),
                              0,
                              resultBuffer.getNumSamples()))
    {
        failToLoad ("Unable to read data from IR file: " + file.getFullPathName());
        return;
    }

    irState.paramIndex = customIRIndex;
    irState.data = std::make_unique<MemoryBlock> (std::move (irData));
    if (file != File {})
    {
        irState.name = file.getFileNameWithoutExtension();
        irState.file = file;
    }
    else
    {
        irState.name = name;
        irState.file = File {};
    }

    irChangedBroadcaster();
    vts.getParameter (irTag)->setValueNotifyingHost (1.0f);

    setMakeupGain ((float) formatReader->sampleRate);

    ScopedLock sl (irMutex);
    convolution.loadImpulseResponse (std::move (resultBuffer), formatReader->sampleRate, dsp::Convolution::Stereo::yes, dsp::Convolution::Trim::yes, dsp::Convolution::Normalise::yes);
}

void AmpIRs::loadIRFromCurrentState()
{
    loadIRFromStream (std::make_unique<MemoryInputStream> (*irState.data, true), irState.name, irState.file, nullptr);
}

namespace
{
const String irNameTag { "ir_custom_name" };
const String irDataTag { "ir_custom_data" };
const String irFileTag { "ir_custom_file" };
} // namespace

std::unique_ptr<XmlElement> AmpIRs::toXML()
{
    auto xml = BaseProcessor::toXML();

    if (irState.data != nullptr)
    {
        xml->setAttribute (irNameTag, irState.name);
        xml->setAttribute (irDataTag, Base64::toBase64 (irState.data->getData(), irState.data->getSize()));
        if (irState.file != File {})
            xml->setAttribute (irFileTag, irState.file.getFullPathName());
    }

    return std::move (xml);
}

void AmpIRs::fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition)
{
    BaseProcessor::fromXML (xml, version, loadPosition);

    using namespace chowdsp::version_literals;
    if (version >= "1.1.4"_v)
    {
        if (xml->hasAttribute (irNameTag) && xml->hasAttribute (irDataTag))
        {
            irState.name = xml->getStringAttribute (irNameTag);
            irState.file = xml->getStringAttribute (irFileTag);

            const auto& irDataRaw = xml->getStringAttribute (irDataTag);
            MemoryOutputStream outStream { (size_t) irDataRaw.length() };
            const auto successfulRead = Base64::convertFromBase64 (outStream, irDataRaw);
            if (! successfulRead)
                vts.getParameter ("ir")->setValueNotifyingHost (0.0f);

            irState.data = std::make_unique<MemoryBlock> (outStream.getMemoryBlock());
            loadIRFromCurrentState();
        }
    }
    else // prior to v1.1.4, we were just saving the IR file path
    {
        auto irFile = File (xml->getStringAttribute ("ir_file"));
        if (irFile.existsAsFile())
            loadIRFromStream (irFile.createInputStream());
    }
}
