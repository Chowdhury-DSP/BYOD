#include "BYOD.h"

void BYOD::addParameters (Parameters& params)
{
}

void BYOD::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void BYOD::releaseResources()
{
}

void BYOD::processAudioBlock (AudioBuffer<float>& buffer)
{
}

// This creates new instances of the plugin
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BYOD();
}
