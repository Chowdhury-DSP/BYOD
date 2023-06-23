#pragma once

#include "JuceProcWrapper.h"
#include "netlist_helpers/CircuitQuantity.h"

enum ProcessorType
{
    Drive,
    Tone,
    Modulation,
    Utility,
    Other,
};

enum class PortType
{
    audio = 0,
    modulation,
    level
};

struct ProcessorUIOptions
{
    Colour backgroundColour = Colours::red;
    Colour powerColour = Colour (0xFFFF4D29);
    std::unique_ptr<Drawable> backgroundImage;
    StringArray paramIDsToSkip;

    struct ProcInfo
    {
        String description;
        StringArray authors;
        String infoLink;
    } info;
};

class BaseProcessor;
class ProcessorEditor;
namespace netlist
{
struct CircuitQuantityList;
}
struct ConnectionInfo
{
    BaseProcessor* startProc;
    int startPort;
    BaseProcessor* endProc;
    int endPort;
};

namespace base_processor_detail
{
using PortTypesVector = chowdsp::SmallVector<PortType, 4>;

template <typename Port, typename PortMapper>
static PortTypesVector initialisePortTypes (PortMapper mapper)
{
    auto portTypes = PortTypesVector (magic_enum::enum_count<Port>(), PortType::audio);
    magic_enum::enum_for_each<Port> ([&portTypes, &mapper] (auto portType)
                                     {
                                         const auto portIndex = *magic_enum::enum_index ((Port) portType);
                                         portTypes[portIndex] = mapper ((Port) portType); });
    return portTypes;
}
} // namespace base_processor_detail

class BaseProcessor : private JuceProcWrapper
{
public:
    using Ptr = std::unique_ptr<BaseProcessor>;

    template <typename Port>
    static constexpr auto defaultPortMapper (Port)
    {
        return PortType::audio;
    }

    template <typename InputPort,
              typename OutputPort,
              typename InputPortMapper = decltype (&defaultPortMapper<InputPort>),
              typename OutputPortMapper = decltype (&defaultPortMapper<OutputPort>)>
    BaseProcessor (const String& name,
                   ParamLayout params,
                   InputPort,
                   OutputPort,
                   UndoManager* um = nullptr,
                   InputPortMapper inputPortMapper = &defaultPortMapper<InputPort>,
                   OutputPortMapper outputPortMapper = &defaultPortMapper<OutputPort>) : JuceProcWrapper (name),
                                                                                         vts (*this, um, Identifier ("Parameters"), std::move (params)),
                                                                                         numInputs (magic_enum::enum_count<InputPort>()),
                                                                                         numOutputs (magic_enum::enum_count<OutputPort>()),
                                                                                         inputPortTypes (base_processor_detail::initialisePortTypes<InputPort> (inputPortMapper)),
                                                                                         outputPortTypes (base_processor_detail::initialisePortTypes<OutputPort> (outputPortMapper))
    {
        std::cout << "Creating processor: " << name << std::endl;
        std::cout << "With input ports: " << std::endl;
        for (size_t i = 0; i < inputPortTypes.size(); ++i)
            std::cout << "    " << magic_enum::enum_name ((InputPort) i) << " (" << magic_enum::enum_name (inputPortTypes[i]) << ")" << std::endl;
        std::cout << "With output ports: " << std::endl;
        for (size_t i = 0; i < outputPortTypes.size(); ++i)
            std::cout << "    " << magic_enum::enum_name ((OutputPort) i) << " (" << magic_enum::enum_name (outputPortTypes[i]) << ")" << std::endl;

        onOffParam = vts.getRawParameterValue ("on_off");

        outputBuffers.resize (jmax (1, numOutputs));
        outputBuffers.fill (nullptr);
        outputConnections.resize (numOutputs);

        inputBuffers.resize (numInputs);
        inputsConnected.resize (0);
        portMagnitudes.resize (numInputs);
    }

    BaseProcessor (const String& name,
                   ParamLayout params,
                   UndoManager* um = nullptr);
    ~BaseProcessor();

    // metadata
    virtual ProcessorType getProcessorType() const = 0;
    const String getName() const override { return JuceProcWrapper::getName(); }

    // audio processing methods
    bool isBypassed() const { return ! static_cast<bool> (onOffParam->load()); }
    void prepareProcessing (double sampleRate, int numSamples);
    void freeInternalMemory();
    void processAudioBlock (AudioBuffer<float>& buffer);

    // methods for working with port input levels
    float getInputLevelDB (int portIndex) const noexcept;
    void resetPortMagnitudes (bool shouldPortMagsBeOn);

    // state save/load methods
    virtual std::unique_ptr<XmlElement> toXML();
    virtual void fromXML (XmlElement* xml, const chowdsp::Version& version, bool loadPosition = true);
    void loadPositionInfoFromXML (XmlElement* xml);

    // interface for processor editors
    AudioProcessorValueTreeState& getVTS() { return vts; }
    ProcessorUIOptions& getUIOptions() { return uiOptions; }
    const ProcessorUIOptions& getUIOptions() const { return uiOptions; }

    /** If a processor changes its UI options after construction it should call this to alert the editor. */
    chowdsp::Broadcaster<void()> uiOptionsChanged;

    /**
     * If your processor has custom UI components, create them here!
     *
     * Return true if the custom components should be ordered before the other components on the UI.
     */
    virtual bool getCustomComponents (OwnedArray<Component>& /*customComps*/, chowdsp::HostContextProvider&) { return false; }

    /** if your processor needs a custom looks and feel, create it here! (with the shared lnfAllocator) */
    virtual LookAndFeel* getCustomLookAndFeel() const { return nullptr; }

    /** add options to the processor's popup menu */
    virtual void addToPopupMenu (PopupMenu& menu);

    /**
     * Sets the editor that is controlling this processor.
     * Called by the editor ONLY!
     */
    void setEditor (ProcessorEditor* procEditor);

    /** Returns the editor as a Component::SafePointer */
    const auto& getEditor() const { return editor; }

    /**
     * Sets the netlist window to use for the processor.
     * INTERNAL USE ONLY
     */
    void setNetlistWindow (std::unique_ptr<Component>&& window) { netlistWindow = std::move (window); }

    /** Returns the netlist circuit quantities, or nullptr if the processor has no circuit quantities. */
    auto* getNetlistCircuitQuantities() { return netlistCircuitQuantities.get(); }

    AudioBuffer<float>& getInputBuffer (int idx = 0) { return inputBuffers.getReference (idx); }
    AudioBuffer<float>* getOutputBuffer (int idx = 0) { return outputBuffers[idx]; }
    const ConnectionInfo& getOutputConnection (int portIdx, int connectionIdx) const { return outputConnections[portIdx].getReference (connectionIdx); }

    int getNumOutputConnections (int portIdx) const { return outputConnections[portIdx].size(); }
    int getNumInputConnections() const { return inputsConnected.size(); };

    int incrementNumInputsReady() { return numInputsReady++; }
    int getNumInputsReady() const { return numInputsReady; }
    void clearNumInputsReady() { numInputsReady = 0; }

    void addConnection (ConnectionInfo&& info);
    void removeConnection (const ConnectionInfo& info);
    virtual void inputConnectionChanged (int /*portIndex*/, bool /*wasConnected*/) {}

    int getNumInputs() const noexcept { return numInputs; }
    int getNumOutputs() const noexcept { return numOutputs; }

    void setPosition (juce::Point<int> pos, Rectangle<int> parentBounds);
    void setPosition (const BaseProcessor& other) { editorPosition = other.editorPosition; }
    juce::Point<int> getPosition (Rectangle<int> parentBounds);

    const auto& getParameters() const { return AudioProcessor::getParameters(); }

    bool isInputModulationPort (int portIndex);
    bool isOutputModulationPort (int portIndex);
    bool isOutputModulationPortConnected();

    const std::vector<String>* getParametersToDisableWhenInputIsConnected (int portIndex) const noexcept;

protected:
    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void releaseMemory() {}
    virtual void processAudio (AudioBuffer<float>& buffer) = 0;

    /** All multi-input or multi-output modules should override this method! */
    virtual void processAudioBypassed (AudioBuffer<float>& /*buffer*/) { jassert (getNumInputs() <= 1 && getNumOutputs() <= 1); }

    /**
     * If a particular parameter should be shown in the module's popup menu
     * rather than the knobs component, then call this method in the module's
     * constructor with the appropriate parameter ID.
     */
    void addPopupMenuParameter (const String& paramID);

    /**
     * If a parameter should be disabled when an input is connected, then
     * call this method in the module's constructor with the appropriate paramIDs.
     */
    void disableWhenInputConnected (const std::initializer_list<String>& paramIDs, int inputPortIndex);

    /** 
     * All modulation signals should be in the range of [-1,1],
     * they can then be modified as needed by the individual module.
     */
    void routeExternalModulation (const std::initializer_list<int>& inputPorts, const std::initializer_list<int>& outputPorts);

    AudioProcessorValueTreeState vts;
    ProcessorUIOptions uiOptions;

    Array<AudioBuffer<float>*> outputBuffers;
    Array<int> inputsConnected;

    chowdsp::SharedLNFAllocator lnfAllocator;
    Component::SafePointer<ProcessorEditor> editor {};

    std::unique_ptr<Component> netlistWindow {};
    std::unique_ptr<netlist::CircuitQuantityList> netlistCircuitQuantities {};

    /**
     * If your processor uses convolution, you can use this shared
     * messaging queue to avoid creating a new background thread
     * for each instance.
     */
    auto& getSharedConvolutionMessageQueue() { return convolutionMessageQueue.get(); }

    enum class BasicInputPort
    {
        AudioInput,
    };

    enum class BasicOutputPort
    {
        AudioOutput,
    };

private:
    std::atomic<float>* onOffParam = nullptr;

    const int numInputs;
    const int numOutputs;

    std::vector<Array<ConnectionInfo>> outputConnections;
    Array<AudioBuffer<float>> inputBuffers;
    int numInputsReady = 0;

    juce::Point<float> editorPosition;

    struct ConvolutionMessageQueue : public dsp::ConvolutionMessageQueue
    {
        ConvolutionMessageQueue() : dsp::ConvolutionMessageQueue (2048) {}
    };
    SharedResourcePointer<ConvolutionMessageQueue> convolutionMessageQueue;

    struct PortMagnitude
    {
        PortMagnitude() = default;
        PortMagnitude (PortMagnitude&&) noexcept {}

        chowdsp::LevelDetector<float> smoother;
        Atomic<float> currentMagnitudeDB;
    };

    bool portMagnitudesOn = false;
    std::vector<PortMagnitude> portMagnitudes;

    StringArray popupMenuParameterIDs;
    OwnedArray<ParameterAttachment> popupMenuParameterAttachments;

    juce::Array<int> inputModulationPorts {};
    juce::Array<int> outputModulationPorts {};
    base_processor_detail::PortTypesVector inputPortTypes;
    base_processor_detail::PortTypesVector outputPortTypes;

    std::unordered_map<int, std::vector<String>> paramsToDisableWhenInputConnected {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};
