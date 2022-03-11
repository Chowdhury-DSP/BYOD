# Creating A New Processor

BYOD is open to public contributions. If you have
a guitar processing effect that you would like to
add to the plugin, please follow this guide!

## Setting Up The Processor

In order to set up a new processor, the processor
must have a name and a category. Currently there are
four available categories: Drive, Tone, Utility, and
Other. For our example here, we'll give our processor
the name "My Effect", and put it in the "Other" category.

Next we'll need to create files for the new processor,
`MyEffect.h` and `MyEffect.cpp`. If these are the only
files we'll need for the processor, they should go into
`src/processors/other`. If more files will be needed,
a separate folder should be made `src/processors/other/my_effect`.

In these files, we'll create a `MyEffect` class, which
will be a subclass of the `BaseProcessor`. Here is a basic
template for the class:

`MyEffect.h`
```cpp
#pragma once

#include "../BaseProcessor.h"

class MyEffect : public BaseProcessor
{
public:
    MyEffect (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return Other; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyEffect)
};
```

`MyEffect.cpp`
```cpp
#include "MyEffect.h"
#include "../ParameterHelpers.h"

MyEffect::MyEffect (UndoManager* um) : BaseProcessor ("My Effect", createParameterLayout(), um)
{
}

AudioProcessorValueTreeState::ParameterLayout MyEffect::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    return { params.begin(), params.end() };
}

void MyEffect::prepare (double sampleRate, int samplesPerBlock)
{
}

void MyEffect::processAudio (AudioBuffer<float>& buffer)
{
}
```

If you don't want to write out all this template code,
you may use the `scripts/new_proc.sh` script.
```bash
$ bash scripts/new_proc.sh <class_name> <processor_type> <processor_name>
```
Note that the `processor_type` must be one of `drive`,
`tone`, `utility`, or `other` (note the lowercase). The
`processor_name` parameter is only needed if it's different
from the `class_name`.

Finally, we'll need to add our new effect to the 
`ProcessorStore`. In `ProcessorStore.cpp`, we need
to include our new header `#include "other/MyEffect.h"`,
and then add the processor to the store map:
```cpp
ProcessorStore::StoreMap ProcessorStore::store = {
    ...
    { "My Effect", &processorFactory<MyEffect> },
    ...
};
```
Now if we build and run the plugin, we should be
able to load our new effect!

## Adding Parameters

To add parameters to our processor, we must first
define the parameters in the `createParameterLayout()`
function. Here are a few examples:
```cpp
AudioProcessorValueTreeState::ParameterLayout MyEffect::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    // add a frequency parameter
    createFreqParam (params, "freq", "Frequency", 20.0f, 20000.0f, 1000.0f, 1000.0f);

    // add a percent parameter
    createPercentParameter (params, "drive", "Drive", 0.5f);

    // add a time parameter
    params.push_back (std::make_unique<VTSParam> ("time_param",
                                                  "Param",
                                                  String(),
                                                  NormalisableRange<float> { 5.0f, 500.0f },
                                                  100.0f,
                                                  &timeMsValToString,
                                                  &stringToTimeMsVal));

    // add a choice parameter
    params.push_back (std::make_unique<AudioParameterChoice> ("choice_param",
                                                              "Choices",
                                                              StringArray { "Option 1", "Option 2" },
                                                              0));

    return { params.begin(), params.end() };
}
```

Next we'll need to create "handles" for each parameter as a member
variable of the class:
```cpp
class MyEffect
{
    ...
private:
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* timeParam = nullptr;
    std::atomic<float>* choiceParam = nullptr;
    ...
};
```
and then initialize each handle in the constructor:
```cpp
MyEffect::MyEffect (UndoManager* um) : BaseProcessor ("My Effect", createParameterLayout(), um)
{
    freqParam = vts.getRawParameterValue ("freq");
    driveParam = vts.getRawParameterValue ("drive");
    timeParam = vts.getRawParameterValue ("time_param");
    choiceParam = vts.getRawParameterValue ("choice_param");
}
```
Currently processors can have a maximum of 5 parameters.
In the future, more parameter may become available.

## Implementing the DSP

To implement the signal processing for our effect, there are
only two methods that we need to implement: `prepare()` and
`processAudio()`. `prepare()` informs the processor of the
sample rate and block size that will be used for processing,
so any values that need to be pre-computed based on the sample
rate, or memory allocation operations that depend on the block
size should be done here.

The actual real-time processing should be done in `processAudio()`.
Here's a dummy example:
```cpp
void MyEffect::processAudio (AudioBuffer<float>& buffer)
{
    // access our parameters
    auto freqValueHz = freqParam->load();
    auto driveValue = driveParam->load();
    auto timeMs = timeParam->load();
    auto choiceIndex = (int) choiceParam->load();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* channelData = buffer.getWritePointer (ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            // do sample processing here...
        }
    }
}
```
Note that since the parameter handles are
[atomic values](https://en.cppreference.com/w/cpp/atomic/atomic),
accessing them is slower than accessing a typical floating
point value, so it is recommended to only access each parameter
once per block.

## Customizing the UI

The GUI for your processor can be customized via the
`ProcessorUIOptions` struct. At minimum, it is recommended
to customize the colours for your processor, and fill in
the processor information:
```cpp
MyEffect::MyEffect (UndoManager* um) : BaseProcessor ("My Effect", createParameterLayout(), um)
{
    ...
    uiOptions.backgroundColour = myBackgroundColour;
    uiOptions.powerColour = myPowerButtonColour;
    uiOptions.info.description = "This is a description of my effect.";
    uiOptions.info.authors = StringArray { "Author Name 1", "Author Name 2" };
}
```
If you would like your processor to have a custom background image,
that can be done by adding your image to the plugin's `BinaryData`
(see `res/CMakeLists.txt`), and then adding it to the `uiOptions`:
```cpp
uiOptions.backgroundImage = Drawable::createFromImageData (BinaryData::background_png, BinaryData::background_pngSize);
```
If you would like your effect's UI to have a custom `LookAndFeel`,
that can be done as follows:
```cpp
class MyEffect : public BaseProcessor
{
    ...
private:
    ...
    SharedResourcePointer<LNFAllocator> lnfAllocator;
};

class MyEffectLookAndFeel : public LookAndFeel_V4
{
    // custom LookAndFeel implementation...
};

MyEffect::MyEffect (UndoManager* um) : BaseProcessor ("My Effect", createParameterLayout(), um)
{
    ...
    uiOptions.lnf = lnfAllocator->getLookAndFeel<MyEffectLookAndFeel>();
}
```

## That's All!

If you have any questions, or run into any issues with the
process outlined here, please create a [GitHub Issue](https://github.com/Chowdhury-DSP/BYOD/issues).

