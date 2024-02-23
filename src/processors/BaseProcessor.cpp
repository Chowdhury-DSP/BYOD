#include "BaseProcessor.h"
#include "gui/pedalboard/editors/ProcessorEditor.h"
#include "netlist_helpers/NetlistViewer.h"
#include "state/ParamForwardManager.h"

BaseProcessor::BaseProcessor (const String& name,
                              ParamLayout params,
                              UndoManager* um) : BaseProcessor (
    name,
    std::move (params),
    BasicInputPort {},
    BasicOutputPort {},
    um,
    [] (auto)
    { return PortType::audio; },
    [] (auto)
    { return PortType::audio; })
{
}

BaseProcessor::BaseProcessor (const String& name,
                              ParamLayout&& params,
                              base_processor_detail::PortTypesVector&& inputPorts,
                              base_processor_detail::PortTypesVector&& outputPorts,
                              UndoManager* um)
    : JuceProcWrapper (name),
      vts (*this, um, Identifier ("Parameters"), std::move (params)),
      numInputs ((int) inputPorts.size()),
      numOutputs ((int) outputPorts.size()),
      inputPortTypes (std::move (inputPorts)),
      outputPortTypes (std::move (outputPorts))
{
    onOffParam = vts.getRawParameterValue ("on_off");

    outputBuffers.resize (jmax (1, numOutputs));
    outputBuffers.fill (nullptr);
    outputConnections.resize ((size_t) numOutputs);

    inputBuffers.resize (numInputs);
    inputsConnected.resize (0);
    portMagnitudes.resize ((size_t) numInputs);
}

BaseProcessor::~BaseProcessor() = default;

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

void BaseProcessor::freeInternalMemory()
{
    releaseMemory();
    for (auto& b : inputBuffers)
        b.setSize (0, 0);
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

    if (netlistCircuitQuantities != nullptr)
    {
        for (auto& quantity : *netlistCircuitQuantities)
        {
            if (chowdsp::AtomicHelpers::compareNegate (quantity.needsUpdate))
                quantity.setter (quantity);
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
    xml->setAttribute (chowdsp::toString (ParamForwardManager::processorSlotIndexTag), forwardingParamsSlotIndex);

    if (netlistCircuitQuantities != nullptr)
    {
        auto circuitXML = std::make_unique<XmlElement> ("circuit_elements");

        for (const auto& quantity : *netlistCircuitQuantities)
            circuitXML->setAttribute (juce::String { quantity.name }, (double) quantity.value);

        xml->addChildElement (circuitXML.release());
    }

    return std::move (xml);
}

void BaseProcessor::fromXML (XmlElement* xml, const chowdsp::Version&, bool loadPosition)
{
    if (xml == nullptr)
        return;

    if (! xml->hasTagName (vts.state.getType()))
        return;

    vts.state = ValueTree::fromXml (*xml); // don't use `replaceState()` otherwise UndoManager will clear

    forwardingParamsSlotIndex = xml->getIntAttribute (chowdsp::toString (ParamForwardManager::processorSlotIndexTag), -1);

    if (loadPosition)
        loadPositionInfoFromXML (xml);

    if (netlistCircuitQuantities != nullptr)
    {
        if (auto* circuitXML = xml->getChildByName ("circuit_elements"))
        {
            for (auto& quantity : *netlistCircuitQuantities)
            {
                const auto name = juce::String { quantity.name };
                if (circuitXML->hasAttribute (name))
                    quantity.value = (float) circuitXML->getDoubleAttribute (name, (double) quantity.defaultValue);
                else
                    quantity.value = quantity.defaultValue;
            }
        }
        else
        {
            for (auto& quantity : *netlistCircuitQuantities)
                quantity.value = quantity.defaultValue;
        }

        for (auto& quantity : *netlistCircuitQuantities)
        {
            quantity.setter (quantity);
            quantity.needsUpdate = false;
        }
    }
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
    outputConnections[(size_t) info.startPort].add (info);

    // make sure the end processor actually has an input port available that we can connect to!
    jassert (info.endProc->inputsConnected.size() + 1 <= info.endProc->numInputs);
    info.endProc->inputsConnected.addUsingDefaultSort (info.endPort);
    info.endProc->inputConnectionChanged (info.endPort, true);
}

void BaseProcessor::removeConnection (const ConnectionInfo& info)
{
    jassert (info.startProc == this);

    auto& connections = outputConnections[(size_t) info.startPort];
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

const std::vector<String>* BaseProcessor::getParametersToDisableWhenInputIsConnected (int portIndex) const noexcept
{
    if (auto iter = paramsToDisableWhenInputConnected.find (portIndex); iter != paramsToDisableWhenInputConnected.end())
        return &(iter->second);

    return nullptr;
}

const std::vector<String>* BaseProcessor::getParametersToEnableWhenInputIsConnected (int portIndex) const noexcept
{
    if (auto iter = paramsToEnableWhenInputConnected.find (portIndex); iter != paramsToEnableWhenInputConnected.end())
        return &(iter->second);

    return nullptr;
}

void BaseProcessor::disableWhenInputConnected (const std::initializer_list<String>& paramIDs, int inputPortIndex)
{
    paramsToDisableWhenInputConnected[inputPortIndex] = paramIDs;
}

void BaseProcessor::enableWhenInputConnected (const std::initializer_list<String>& paramIDs, int inputPortIndex)
{
    paramsToEnableWhenInputConnected[inputPortIndex] = paramIDs;
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

    auto addChoiceParam = [this, &menu, &itemID] (AudioParameterChoice* param, bool isEnabled)
    {
        menu.addSectionHeader (param->name);

        auto* attachment = popupMenuParameterAttachments.add (std::make_unique<ParameterAttachment> (
            *param, [=] (float) {}, vts.undoManager));

        for (const auto [index, paramChoice] : sst::cpputils::enumerate (param->choices))
        {
            PopupMenu::Item paramItem;
            paramItem.itemID = ++itemID;
            paramItem.text = paramChoice;
            paramItem.action = [attachment, newParamVal = (float) index]
            {
                attachment->setValueAsCompleteGesture (newParamVal);
            };
            paramItem.colour = (param->getIndex() == (int) index) ? uiOptions.powerColour : Colours::white;
            paramItem.isEnabled = isEnabled;

            menu.addItem (paramItem);
        }
    };

    auto addBoolParam = [this, &itemID, &menu] (AudioParameterBool* param, bool isEnabled)
    {
        auto* attachment = popupMenuParameterAttachments.add (std::make_unique<ParameterAttachment> (
            *param, [] (float) {}, vts.undoManager));

        PopupMenu::Item paramItem;
        paramItem.itemID = ++itemID;
        paramItem.text = param->name;
        paramItem.action = [attachment, param]
        { attachment->setValueAsCompleteGesture (param->get() ? 0.0f : 1.0f); };
        paramItem.colour = param->get() ? uiOptions.powerColour : Colours::white;
        paramItem.isEnabled = isEnabled;

        menu.addItem (paramItem);
    };

    for (const auto& paramID : popupMenuParameterIDs)
    {
        bool isEnabled = true;
        for (int i = 0; i < getNumInputs(); ++i)
        {
            if (! inputsConnected.contains (i))
                continue;

            if (auto* paramIDsToDisable = getParametersToDisableWhenInputIsConnected (i);
                paramIDsToDisable != nullptr
                && std::find (paramIDsToDisable->begin(), paramIDsToDisable->end(), paramID)
                       != paramIDsToDisable->end())
                isEnabled = false;
        }

        auto* param = vts.getParameter (paramID);
        if (auto* choiceParam = dynamic_cast<AudioParameterChoice*> (param))
            addChoiceParam (choiceParam, isEnabled);
        else if (auto* boolParam = dynamic_cast<AudioParameterBool*> (param))
            addBoolParam (boolParam, isEnabled);
        else
            jassertfalse; // popup menu items are not supported for this type of parameter by default!

        menu.addSeparator();
    }

    if (netlistCircuitQuantities != nullptr)
    {
        menu.addItem (netlist::createNetlistViewerPopupMenuItem (*this));
        menu.addSeparator();
    }
}

void BaseProcessor::setEditor (ProcessorEditor* procEditor)
{
    editor = procEditor;
}

PortType BaseProcessor::getInputPortType (int portIndex) const
{
    return inputPortTypes[(size_t) portIndex];
}

PortType BaseProcessor::getOutputPortType (int portIndex) const
{
    return outputPortTypes[(size_t) portIndex];
}

void BaseProcessor::setPosition (juce::Point<int> pos, juce::Rectangle<int> parentBounds)
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

juce::Point<int> BaseProcessor::getPosition (juce::Rectangle<int> parentBounds)
{
    return (editorPosition * juce::Point { (float) parentBounds.getWidth(), (float) parentBounds.getHeight() }).toInt();
}

bool BaseProcessor::isOutputModulationPortConnected()
{
    if (getProcessorType() != Modulation)
        return false;

    for (const auto& connections : outputConnections)
    {
        for (auto info : connections)
        {
            if (getOutputPortType (info.startPort) == PortType::modulation)
                return true;
        }
    }
    return false;
}

String BaseProcessor::getTooltipForPort (int portIndex, bool isInput)
{
    const auto portType = isInput ? getInputPortType (portIndex) : getOutputPortType (portIndex);
    String tooltip;
    if (portType == PortType::audio)
        tooltip = "Audio";
    else if (portType == PortType::modulation)
        tooltip = "Modulation";
    else if (portType == PortType::level)
        tooltip = "Level";
    return tooltip + (isInput ? " Input" : " Output");
}
