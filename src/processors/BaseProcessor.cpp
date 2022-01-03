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

    outputBuffers.resize (jmax (1, numOutputs));
    outputBuffers.fill (nullptr);
    outputConnections.resize (numOutputs);

    inputBuffers.resize (numInputs);
    inputsConnected.resize (0);
    portMagnitudes.resize (numInputs);

    uiOptions.lnf = lnfAllocator->getLookAndFeel<ProcessorLNF>();
}

void BaseProcessor::prepareProcessing (double sampleRate, int numSamples)
{
    prepare (sampleRate, numSamples);

    for (auto& b : inputBuffers)
    {
        b.setSize (2, numSamples);
        b.clear();
    }

    for (auto& mag : portMagnitudes)
    {
        mag.smoother.prepare ({ sampleRate, (uint32) numSamples, 1 });
        mag.smoother.setParameters (15.0f, 150.0f);
        mag.currentMagnitudeDB = -100.0f;
    }
}

void BaseProcessor::processAudioBlock (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    auto updateBufferMag = [&] (const AudioBuffer<float>& inBuffer, int inputIndex)
    {
        auto rmsAvg = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            rmsAvg += Decibels::gainToDecibels (inBuffer.getRMSLevel (ch, 0, numSamples)); // @TODO: not sure if getRMSLevel is optimized enough...

        rmsAvg /= (float) numChannels;

        auto& portMag = portMagnitudes[(size_t) inputIndex];
        float curMag = 0.0f;
        for (int n = 0; n < numSamples; ++n)
            curMag = portMag.smoother.processSample (rmsAvg);
        portMag.currentMagnitudeDB.set (curMag);
    };

    // track input levels
    if (numInputs == 1)
    {
        updateBufferMag (buffer, 0);
    }
    else if (numInputs > 1)
    {
        for (int i = 0; i < numInputs; ++i)
            updateBufferMag (getInputBuffer (i), i);
    }

    if (isBypassed())
        processAudioBypassed (buffer);
    else
        processAudio (buffer);
}

float BaseProcessor::getInputLevelDB (int portIndex) const noexcept
{
    jassert (isPositiveAndBelow (portIndex, numInputs));
    return portMagnitudes[(size_t) portIndex].currentMagnitudeDB.get();
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

void BaseProcessor::addConnection (ConnectionInfo&& info)
{
    jassert (info.startProc == this);
    outputConnections[info.startPort].add (info);

    info.endProc->inputsConnected.addUsingDefaultSort (info.endPort);
}

void BaseProcessor::removeConnection (const ConnectionInfo& info)
{
    jassert (info.startProc == this);

    auto& connections = outputConnections[info.startPort];
    for (int cIdx = 0; cIdx < connections.size(); ++cIdx)
    {
        if (connections[cIdx].endProc == info.endProc && connections[cIdx].endPort == info.endPort)
        {
            connections.remove (cIdx);
            info.endProc->inputsConnected.removeFirstMatchingValue (info.endPort);
            info.endProc->inputBuffers[info.endPort].clear();
            break;
        }
    }
}

void BaseProcessor::setPosition (Point<int> pos, Rectangle<int> parentBounds)
{
    if (parentBounds.getWidth() <= 0 || parentBounds.getHeight() <= 0)
        return; // out of bounds!

    if (pos.x <= -50 || pos.y <= -50)
        return; // out of bounds!

    editorPosition = pos.toFloat() / Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() };

    // limit so we can't go off-screen!
    editorPosition.x = jlimit (0.0f, 0.9f, editorPosition.x);
    editorPosition.y = jlimit (0.0f, 0.9f, editorPosition.y);
}

Point<int> BaseProcessor::getPosition (Rectangle<int> parentBounds)
{
    return (editorPosition * Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() }).toInt();
}
