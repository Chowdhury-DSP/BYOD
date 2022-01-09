#pragma once

#include "JuceProcWrapper.h"
#include "gui/utils/LookAndFeels.h"

enum ProcessorType
{
    Drive,
    Tone,
    Utility,
    Other,
};

struct ProcessorUIOptions
{
    Colour backgroundColour = Colours::red;
    Colour powerColour = Colour (0xFFFF4D29);
    std::unique_ptr<Drawable> backgroundImage;
    LookAndFeel* lnf = nullptr;

    struct ProcInfo
    {
        String description;
        StringArray authors;
        String infoLink;
    } info;
};

class BaseProcessor;
struct ConnectionInfo
{
    BaseProcessor* startProc;
    int startPort;
    BaseProcessor* endProc;
    int endPort;
};

class BaseProcessor : private JuceProcWrapper
{
public:
    using Ptr = std::unique_ptr<BaseProcessor>;

    BaseProcessor (const String& name,
                   AudioProcessorValueTreeState::ParameterLayout params,
                   UndoManager* um = nullptr,
                   int nInputs = 1,
                   int nOutputs = 1);

    // metadata
    virtual ProcessorType getProcessorType() const = 0;
    const String getName() const override { return JuceProcWrapper::getName(); }

    // audio processing methods
    bool isBypassed() const { return ! static_cast<bool> (onOffParam->load()); }
    void prepareProcessing (double sampleRate, int numSamples);
    void processAudioBlock (AudioBuffer<float>& buffer);

    // methods for working with port input levels
    float getInputLevelDB (int portIndex) const noexcept;
    void resetPortMagnitudes (bool shouldPortMagsBeOn);

    // state save/load methods
    virtual std::unique_ptr<XmlElement> toXML();
    virtual void fromXML (XmlElement* xml);

    // interface for processor editors
    AudioProcessorValueTreeState& getVTS() { return vts; }
    const ProcessorUIOptions& getUIOptions() const { return uiOptions; }

    /** if your processor has custom UI components, create them here! */
    virtual void getCustomComponents (OwnedArray<Component>& /*customComps*/) {}

    /** add options to the processor's popup menu */
    virtual void addToPopupMenu (PopupMenu& /*menu*/) {}

    AudioBuffer<float>& getInputBuffer (int idx = 0) { return inputBuffers.getReference (idx); }
    AudioBuffer<float>* getOutputBuffer (int idx = 0) { return outputBuffers[idx]; }

    BaseProcessor* getOutputProcessor (int portIdx, int connectionIdx) { return outputConnections[portIdx][connectionIdx].endProc; }
    const ConnectionInfo& getOutputConnection (int portIdx, int connectionIdx) const { return outputConnections[portIdx].getReference (connectionIdx); }

    int getNumOutputConnections (int portIdx) const { return outputConnections[portIdx].size(); }
    int getNumInputConnections() const { return inputsConnected.size(); };

    int getNextInputIdx() { return inputsConnected[inputIdx++]; }
    int getNumInputsReady() const { return inputIdx; }
    void clearInputIdx() { inputIdx = 0; }

    void addConnection (ConnectionInfo&& info);
    void removeConnection (const ConnectionInfo& info);

    int getNumInputs() const noexcept { return numInputs; }
    int getNumOutputs() const noexcept { return numOutputs; }

    void setPosition (Point<int> pos, Rectangle<int> parentBounds);
    void setPosition (const BaseProcessor& other) { editorPosition = other.editorPosition; }
    Point<int> getPosition (Rectangle<int> parentBounds);

    const auto& getParameters() const { return AudioProcessor::getParameters(); }

protected:
    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void processAudio (AudioBuffer<float>& buffer) = 0;
    virtual void processAudioBypassed (AudioBuffer<float>& /*buffer*/) {}

    AudioProcessorValueTreeState vts;
    ProcessorUIOptions uiOptions;

    Array<AudioBuffer<float>*> outputBuffers;
    Array<int> inputsConnected;

    chowdsp::SharedLNFAllocator lnfAllocator;

    struct ConvolutionMessageQueue : public dsp::ConvolutionMessageQueue
    {
        ConvolutionMessageQueue() : dsp::ConvolutionMessageQueue (2048) {}
    };

    auto& getSharedConvolutionMessageQueue() { return convolutionMessageQueue.get(); }
    SharedResourcePointer<ConvolutionMessageQueue> convolutionMessageQueue;

private:
    std::atomic<float>* onOffParam = nullptr;

    const int numInputs;
    const int numOutputs;

    std::vector<Array<ConnectionInfo>> outputConnections;
    Array<AudioBuffer<float>> inputBuffers;
    int inputIdx = 0;

    Point<float> editorPosition;

    struct PortMagnitude
    {
        PortMagnitude() = default;
        PortMagnitude (PortMagnitude&&) noexcept {}

        chowdsp::LevelDetector<float> smoother;
        Atomic<float> currentMagnitudeDB;
    };

    bool portMagnitudesOn = false;
    std::vector<PortMagnitude> portMagnitudes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};
