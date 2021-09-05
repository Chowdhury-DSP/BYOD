#pragma once

#include <pch.h>

/** Base class to override (mostly) unused juce::AudioProcessor functions */
class JuceProcWrapper : public AudioProcessor
{
public:
    JuceProcWrapper (String name = String()) : name (name) {}

    const String getName() const override { return name; }

    double getTailLengthSeconds() const override { return 0.0; }

    void prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/) override {}
    void releaseResources() override {}

    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Woverloaded-virtual")
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override {}
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE

    bool acceptsMidi() const final { return false; }
    bool producesMidi() const final { return false; }

    AudioProcessorEditor* createEditor() final { return nullptr; }
    bool hasEditor() const final { return false; }

    int getNumPrograms() final { return 0; }
    void setCurrentProgram (int /*index*/) final {}
    int getCurrentProgram() final { return 0; }

    const String getProgramName (int /*index*/) final { return {}; }
    void changeProgramName (int /*index*/, const String& /*newName*/) final {}

    void getStateInformation (MemoryBlock& /*destData*/) final {}
    void setStateInformation (const void* /*data*/, int /*sizeInBytes*/) final {}

private:
    String name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceProcWrapper)
};
