#include "BaseProcessor.h"

BaseProcessor::BaseProcessor (const String& name,
                              ParamLayout params,
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
    auto updateBufferMag = [&] (const AudioBuffer<float>& inBuffer, int inputIndex)
    {
        const auto inBufferNumChannels = inBuffer.getNumChannels();
        const auto inBufferNumSamples = inBuffer.getNumSamples();

        auto rmsAvg = 0.0f;
        for (int ch = 0; ch < inBufferNumChannels; ++ch)
            rmsAvg += Decibels::gainToDecibels (inBuffer.getRMSLevel (ch, 0, inBufferNumSamples)); // @TODO: not sure if getRMSLevel is optimized enough...

        rmsAvg /= (float) inBufferNumChannels;

        auto& portMag = portMagnitudes[(size_t) inputIndex];
        float curMag = 0.0f;
        for (int n = 0; n < inBufferNumSamples; ++n)
            curMag = portMag.smoother.processSample (rmsAvg);
        portMag.currentMagnitudeDB.set (curMag);
    };

    if (portMagnitudesOn) // track input levels
    {
        if (numInputs == 1)
        {
            updateBufferMag (buffer, 0);
        }
        else if (numInputs > 1)
        {
            for (int i = 0; i < numInputs; ++i)
                updateBufferMag (getInputBuffer (i), i);
        }
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

void BaseProcessor::resetPortMagnitudes (bool shouldPortMagsBeOn)
{
    portMagnitudesOn = shouldPortMagsBeOn;

    for (auto& mag : portMagnitudes)
    {
        mag.smoother.reset();
        mag.currentMagnitudeDB = -100.0f;
    }
}

std::unique_ptr<XmlElement> BaseProcessor::toXML()
{
    auto state = vts.copyState();
    auto xml = state.createXml();

    xml->setAttribute ("x_pos", (double) editorPosition.x);
    xml->setAttribute ("y_pos", (double) editorPosition.y);

    return std::move (xml);
}

void BaseProcessor::fromXML (XmlElement* xml, const chowdsp::Version&, bool loadPosition)
{
    if (xml == nullptr)
        return;

    if (! xml->hasTagName (vts.state.getType()))
        return;

    vts.state = ValueTree::fromXml (*xml); // don't use `replaceState()` otherwise UndoManager will clear

    if (loadPosition)
        loadPositionInfoFromXML (xml);
}

void BaseProcessor::loadPositionInfoFromXML (XmlElement* xml)
{
    if (xml == nullptr)
        return;

    if (! xml->hasTagName (vts.state.getType()))
        return;

    auto xPos = (float) xml->getDoubleAttribute ("x_pos");
    auto yPos = (float) xml->getDoubleAttribute ("y_pos");
    editorPosition = juce::Point { xPos, yPos };
}

void BaseProcessor::addConnection (ConnectionInfo&& info)
{
    jassert (info.startProc == this);
    outputConnections[info.startPort].add (info);

    info.endProc->inputsConnected.addUsingDefaultSort (info.endPort);
    info.endProc->inputConnectionChanged (info.endPort, true);
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
            info.endProc->inputConnectionChanged (info.endPort, false);
            break;
        }
    }
}

void BaseProcessor::addPopupMenuParameter (const String& paramID)
{
    uiOptions.paramIDsToSkip.addIfNotAlreadyThere (paramID);
    popupMenuParameterIDs.addIfNotAlreadyThere (paramID);
}

void BaseProcessor::addToPopupMenu (PopupMenu& menu)
{
    popupMenuParameterAttachments.clear();
    int itemID = 0;

    auto addChoiceParam = [this, &menu, &itemID] (AudioParameterChoice* param)
    {
        menu.addSectionHeader (param->name);

        auto* attachment = popupMenuParameterAttachments.add (std::make_unique<ParameterAttachment> (
            *param, [=] (float) {}, vts.undoManager));

        for (const auto [index, paramChoice] : sst::cpputils::enumerate (param->choices))
        {
            PopupMenu::Item paramItem;
            paramItem.itemID = ++itemID;
            paramItem.text = paramChoice;
            paramItem.action = [attachment, newParamVal = param->convertTo0to1 ((float) index)]
            {
                attachment->setValueAsCompleteGesture (newParamVal);
            };
            paramItem.colour = (param->getIndex() == (int) index) ? uiOptions.powerColour : Colours::white;

            menu.addItem (paramItem);
        }
    };

    auto addBoolParam = [this, &itemID, &menu] (AudioParameterBool* param)
    {
        auto* attachment = popupMenuParameterAttachments.add (std::make_unique<ParameterAttachment> (
            *param, [] (float) {}, vts.undoManager));

        PopupMenu::Item paramItem;
        paramItem.itemID = ++itemID;
        paramItem.text = param->name;
        paramItem.action = [attachment, param]
        { attachment->setValueAsCompleteGesture (param->get() ? 0.0f : 1.0f); };
        paramItem.colour = param->get() ? uiOptions.powerColour : Colours::white;

        menu.addItem (paramItem);
    };

    for (const auto& paramID : popupMenuParameterIDs)
    {
        auto* param = vts.getParameter (paramID);

        if (auto* choiceParam = dynamic_cast<AudioParameterChoice*> (param))
            addChoiceParam (choiceParam);
        else if (auto* boolParam = dynamic_cast<AudioParameterBool*> (param))
            addBoolParam (boolParam);
        else
            jassertfalse; // popup menu items are not supported for this type of parameter by default!

        menu.addSeparator();
    }
}

void BaseProcessor::setPosition (juce::Point<int> pos, Rectangle<int> parentBounds)
{
    if (parentBounds.getWidth() <= 0 || parentBounds.getHeight() <= 0)
        return; // out of bounds!

    if (pos.x <= -50 || pos.y <= -50)
        return; // out of bounds!

    editorPosition = pos.toFloat() / juce::Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() };

    // limit so we can't go off-screen!
    editorPosition.x = jlimit (0.0f, 0.9f, editorPosition.x);
    editorPosition.y = jlimit (0.0f, 0.9f, editorPosition.y);
}

juce::Point<int> BaseProcessor::getPosition (Rectangle<int> parentBounds)
{
    return (editorPosition * juce::Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() }).toInt();
}

bool BaseProcessor::hasModulationPorts()
{
    return modulationPorts;
}

void BaseProcessor::routeExternalModulation (int inputPort, int outputPort)
{
    inputModulationPort = inputPort;
    outputModulationPort = outputPort;

    modulationPorts = true;
}

int BaseProcessor::getInputModulationPort()
{
    return inputModulationPort;
}

int BaseProcessor::getOutputModulationPort()
{
    return outputModulationPort;
}
