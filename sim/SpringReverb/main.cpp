#include "SpringReverb.h"

namespace
{
constexpr double fs = 48000.0;
constexpr int blockSize = 512;
}

void processAudio (AudioBuffer<float>& buffer, SpringReverb& proc)
{
    int sampleIdx = 0;
    for (; sampleIdx < buffer.getNumSamples(); sampleIdx += blockSize)
    {
        AudioBuffer<float> shortBuffer (buffer.getArrayOfWritePointers(), 2, sampleIdx, blockSize);
        proc.processBlock (shortBuffer);
    }

    while (true)
    {
        auto level = buffer.getRMSLevel (0, buffer.getNumSamples() - blockSize / 4, blockSize / 4);
        if (level < 0.0001f)
            break;

        buffer.setSize (2, buffer.getNumSamples() + blockSize, true);
        AudioBuffer<float> shortBuffer (buffer.getArrayOfWritePointers(), 2, sampleIdx, blockSize);
        proc.processBlock (shortBuffer);

        sampleIdx += blockSize;
    }
}

int main (int /*argc*/, char* /*argv*/[])
{
    std::cout << "Running spring reverb simulation..." << std::endl;

    ScopedJuceInitialiser_GUI scopedJuce;

    // create impulse
    AudioBuffer<float> audio (2, 2 * blockSize);
    audio.setSample (0, 0, 1.0f);
    audio.setSample (1, 0, 1.0f);

    // process
    SpringReverb proc;
    proc.prepare (fs, blockSize);
    processAudio (audio, proc);

    // write to file
    AudioFormatManager formatManager;
    formatManager.registerFormat (new WavAudioFormat(), true);

    File file { "./out_file.wav" };
    file.deleteRecursively();
    auto fStream = std::make_unique<FileOutputStream> (file);
    if (fStream->failedToOpen())
    {
        std::cout << "Unable to open out_file!" << std::endl;
        return 1;
    }

    auto* format = formatManager.findFormatForFileExtension (file.getFileExtension());
    auto bitDepth = format->getPossibleBitDepths().getLast();
    std::unique_ptr<AudioFormatWriter> writer (format->createWriterFor (fStream.get(), fs, 2, bitDepth, {}, 0));
    if (writer == nullptr)
    {
        std::cout << "Unable to create writer!" << std::endl;
        return 1;
    }
    else
    {    
        fStream.release();
    }

    return writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());

    return 0;
}
