#include "GuitarMLFilterDesigner.h"
#include "../BYOD.h"

GuitarMLFilterDesigner::GuitarMLFilterDesigner()
{
    this->commandOption = "--guitarml-filter";
    this->argumentDescription = "--guitarml-filter";
    this->shortDescription = "Generates files for GuitarML filter design";
    this->longDescription = "";
    this->command = [=] (const ArgumentList& args)
    { generateFiles (args); };
}

void GuitarMLFilterDesigner::generateFiles ([[maybe_unused]] const ArgumentList& args)
{
    static constexpr int bufferSize = 8192;
    chowdsp::AudioFileSaveLoadHelper saveLoadHelper;

    chowdsp::Noise<float> noiseGenerator;
    noiseGenerator.setGainDecibels (0.0f);
    noiseGenerator.setSeed (123);
    noiseGenerator.setNoiseType (chowdsp::Noise<float>::NoiseType::Normal);

    const auto factory = ProcessorStore::getStoreMap().at ("GuitarML");
    const auto processFile = [&factory,
                              &saveLoadHelper,
                              &noiseGenerator] (const juce::String& filePath, double sampleRate)
    {
        const auto file = File { filePath };
        std::cout << "Rendering file " << file.getFullPathName() << " at sample rate " << sampleRate << std::endl;

        noiseGenerator.prepare ({ sampleRate, (uint32_t) bufferSize, 1 });

        auto processor = factory (nullptr);
        processor->getParameters()[3]->setValueNotifyingHost (1.0f);
        processor->prepareProcessing (sampleRate, bufferSize);

        AudioBuffer<float> buffer { 1, bufferSize };
        buffer.clear();
        auto&& block = dsp::AudioBlock<float> { buffer };

        noiseGenerator.process(dsp::ProcessContextReplacing<float> { block });
        buffer.clear();
        noiseGenerator.process(dsp::ProcessContextReplacing<float> { block });
        processor->processAudioBlock (buffer);

        file.deleteFile();
        saveLoadHelper.saveBufferToFile (filePath, buffer, sampleRate);
    };

    processFile ("noise_48k.wav", 48000.0);
    processFile ("noise_96k.wav", 96000.0);
    processFile ("noise_192k.wav", 192000.0);
}
