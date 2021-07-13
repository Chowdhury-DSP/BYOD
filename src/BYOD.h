#pragma once

#include "presets/PresetManager.h"
#include "processors/ProcessorChain.h"
#include "processors/ProcessorStore.h"

class BYOD : public chowdsp::PluginBase<BYOD>
{
public:
    BYOD();

    static void addParameters (Parameters& params);
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processAudioBlock (AudioBuffer<float>& buffer) override;

    AudioProcessorEditor* createEditor() override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;

    ProcessorChain& getProcChain() { return procs; }
    AudioProcessorValueTreeState& getVTS() { return vts; }
    PresetManager& getPresetManager() { return presetManager; }

private:
    ProcessorStore procStore;
    ProcessorChain procs;

    PresetManager presetManager;
    UndoManager undoManager { 300000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BYOD)
};
