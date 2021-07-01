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
    } info;
};

class BaseProcessor : private JuceProcWrapper
{
public:
    using Ptr = std::unique_ptr<BaseProcessor>;

    BaseProcessor (const String& name,
                   AudioProcessorValueTreeState::ParameterLayout params,
                   UndoManager* um = nullptr);

    // metadata
    virtual ProcessorType getProcessorType() const = 0;
    const String getName() const override { return JuceProcWrapper::getName(); }

    // audio processing methods
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

protected:
    AudioProcessorValueTreeState vts;
    ProcessorUIOptions uiOptions;

    std::atomic<float>* onOffParam = nullptr;

    SharedResourcePointer<LNFAllocator> lnfAllocator;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};
