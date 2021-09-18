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
    void prepareInputBuffer (int numSamples) { inputBuffer.setSize (2, numSamples); }
    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void processAudio (AudioBuffer<float>& buffer) = 0;

    // bypass methods
    bool isBypassed() const { return ! static_cast<bool> (onOffParam->load()); }
    virtual void processAudioBypassed (AudioBuffer<float>& /*buffer*/) {}

    // state save/load methods
    virtual std::unique_ptr<XmlElement> toXML();
    virtual void fromXML (XmlElement* xml);

    // interface for processor editors
    AudioProcessorValueTreeState& getVTS() { return vts; }
    const ProcessorUIOptions& getUIOptions() const { return uiOptions; }

    /** if your processor has custom UI componenets, create them here! */
    virtual void getCustomComponents (OwnedArray<Component>& /*customComps*/) {}

    /** if your processor can't pass a unit test (for a justifiable reason) say so here! */
    virtual StringArray getTestsToSkip() const { return {}; }

    AudioBuffer<float>& getInputBuffer() { return inputBuffer; }
    AudioBuffer<float>& getOutputBuffer() { return *outputBuffer; }
    BaseProcessor* getOutputProcessor (int portIdx, int connectionIdx) { return outputProcessors[portIdx][connectionIdx]; }
    int getNumOutputProcessors (int portIdx) const { return outputProcessors[portIdx].size(); }

    void addOutputProcessor (BaseProcessor* proc, int portIdx) { outputProcessors[portIdx].addIfNotAlreadyThere (proc); }
    void removeOutputProcessor (BaseProcessor* proc, int portIdx) { outputProcessors[portIdx].remove (outputProcessors[portIdx].indexOf (proc)); }

    int getNumInputs() const noexcept { return numInputs; }
    int getNumOutputs() const noexcept { return numOutputs; }

    void setPosition (Point<int> pos, Rectangle<int> parentBounds);
    Point<int> getPosition (Rectangle<int> parentBounds);

protected:
    AudioProcessorValueTreeState vts;
    ProcessorUIOptions uiOptions;

    AudioBuffer<float>* outputBuffer = nullptr;

    SharedResourcePointer<LNFAllocator> lnfAllocator;

private:
    std::atomic<float>* onOffParam = nullptr;

    const int numInputs;
    const int numOutputs;

    std::vector<Array<BaseProcessor*>> outputProcessors;
    Array<AudioBuffer<float>> bufferArray;
    AudioBuffer<float> inputBuffer;

    Point<float> editorPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};
