#include "ProcessorChain.h"
#include "../ParameterHelpers.h"
#include "ProcessorChainActionHelper.h"
#include "ProcessorChainStateHelper.h"
#include "processors/chain/ChainIOProcessor.h"

namespace
{
[[maybe_unused]] void printBufferLevels (const AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto level = buffer.getRMSLevel (ch, 0, numSamples);
        std::cout << "Level for channel: " << ch << ": " << level << std::endl;
    }
}
} // namespace

ProcessorChain::ProcessorChain (ProcessorStore& store,
                                AudioProcessorValueTreeState& vts,
                                std::unique_ptr<chowdsp::PresetManager>& presetMgr) : procStore (store),
                                                                                      um (vts.undoManager),
                                                                                      inputProcessor (um),
                                                                                      outputProcessor (um),
                                                                                      ioProcessor (vts),
                                                                                      presetManager (presetMgr)
{
    actionHelper = std::make_unique<ProcessorChainActionHelper> (*this);
    stateHelper = std::make_unique<ProcessorChainStateHelper> (*this);

    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChainActionHelper::addProcessor, actionHelper.get(), _1);
    procStore.replaceProcessorCallback = std::bind (&ProcessorChainActionHelper::replaceProcessor, actionHelper.get(), _1, _2);
}

ProcessorChain::~ProcessorChain() = default;

void ProcessorChain::createParameters (Parameters& params)
{
    ChainIOProcessor::createParameters (params);
}

void ProcessorChain::initializeProcessors()
{
    const auto osFactor = ioProcessor.getOversamplingFactor();
    const double osSampleRate = mySampleRate * osFactor;
    const int osSamplesPerBlock = mySamplesPerBlock * osFactor;

    auto prepProcessor = [=] (BaseProcessor& proc)
    {
        proc.prepare (osSampleRate, osSamplesPerBlock);
        proc.prepareInputBuffers (osSamplesPerBlock);
    };

    prepProcessor (inputProcessor);
    prepProcessor (outputProcessor);

    for (auto* processor : procs)
        prepProcessor (*processor);
}

void ProcessorChain::prepare (double sampleRate, int samplesPerBlock)
{
    mySampleRate = sampleRate;
    mySamplesPerBlock = samplesPerBlock;

    inputBuffer.setSize (2, samplesPerBlock * 16); // allocate extra space for upsampled buffers

    ioProcessor.prepare (sampleRate, samplesPerBlock);
    initializeProcessors();
}

void ProcessorChain::runProcessor (BaseProcessor* proc, AudioBuffer<float>& buffer, bool& outProcessed)
{
    int nextNumProcs = 0;
    const int numOutputs = proc->getNumOutputs();
    for (int i = 0; i < numOutputs; ++i)
    {
        const int numOutProcs = proc->getNumOutputConnections (i);
        for (int j = 0; j < numOutProcs; ++j)
            nextNumProcs += 1;
    }

    if (proc == &outputProcessor)
    {
        proc->processAudio (buffer);
        outProcessed = true;
        return;
    }

    if (nextNumProcs == 0)
        return;

    if (proc->isBypassed())
        proc->processAudioBypassed (buffer);
    else
        proc->processAudio (buffer);

    auto processBuffer = [&] (BaseProcessor* nextProc, AudioBuffer<float>& nextBuffer)
    {
        int nextNumInputs = nextProc->getNumInputs();
        if (nextNumProcs == 1 && nextNumInputs == 1)
        {
            runProcessor (nextProc, nextBuffer, outProcessed);
        }
        else if (nextNumProcs > 1 && nextNumInputs == 1)
        {
            auto& copyNextBuffer = nextProc->getInputBuffer();
            copyNextBuffer.makeCopyOf (nextBuffer, true);
            runProcessor (nextProc, copyNextBuffer, outProcessed);
        }
        else
        {
            int inputIdx = nextProc->getNextInputIdx();
            auto& copyNextBuffer = nextProc->getInputBuffer (inputIdx);
            copyNextBuffer.makeCopyOf (nextBuffer, true);

            if (nextProc->getNumInputsReady() < nextProc->getNumInputConnections())
                return; // not all the inputs are ready yet...

            runProcessor (nextProc, copyNextBuffer, outProcessed);
            nextProc->clearInputIdx();
        }
    };

    for (int i = numOutputs - 1; i >= 0; --i)
    {
        auto* outBuffer = proc->getOutputBuffer (i);
        if (outBuffer == nullptr)
            outBuffer = &buffer;

        const int numOutProcs = proc->getNumOutputConnections (i);
        for (int j = numOutProcs - 1; j >= 0; --j)
        {
            auto* nextProc = proc->getOutputProcessor (i, j);
            processBuffer (nextProc, *outBuffer);

            nextNumProcs -= 1;
        }
    }
}

void ProcessorChain::processAudio (AudioBuffer<float>& buffer)
{
    SpinLock::ScopedTryLockType tryProcessingLock (processingLock);
    if (! tryProcessingLock.isLocked())
        return;

    // process input (oversampling, input gain, etc)
    bool sampleRateChange = false;
    auto osBlock = ioProcessor.processAudioInput (buffer, sampleRateChange);
    if (sampleRateChange)
        initializeProcessors();

    const auto osNumSamples = (int) osBlock.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    // process mono or stereo buffer?
    {
        inputBuffer.setSize (2, osNumSamples, false, false, true);
        inputBuffer.clear();

        for (int ch = 0; ch < numChannels; ++ch)
            inputBuffer.copyFrom (ch, 0, osBlock.getChannelPointer ((size_t) ch), osNumSamples);
    }

    // run processing chain
    bool outProcessed = false;
    runProcessor (&inputProcessor, inputBuffer, outProcessed);

    if (outProcessed)
    {
        // get last block from output processor
        auto& outBuffer = *outputProcessor.getOutputBuffer();
        for (int ch = 0; ch < numChannels; ++ch)
            FloatVectorOperations::copy (osBlock.getChannelPointer ((size_t) ch),
                                         outBuffer.getReadPointer (ch),
                                         osNumSamples);
    }
    else
    {
        // output processor is not connected!
        osBlock.clear();
    }

    // do output processing (downsampling, output gain)
    ioProcessor.processAudioOutput (buffer);
}

void ProcessorChain::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
    jassert (presetManager != nullptr);

    if (! presetManager->getIsDirty())
        presetManager->setIsDirty (true);
}
