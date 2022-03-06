#include "ProcessorChain.h"
#include "ProcessorChainActionHelper.h"
#include "ProcessorChainPortMagnitudesHelper.h"
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
                                std::unique_ptr<chowdsp::PresetManager>& presetMgr,
                                std::function<void (int)>&& latencyChangedCallback) : procStore (store),
                                                                                      um (vts.undoManager),
                                                                                      inputProcessor (um),
                                                                                      outputProcessor (um),
                                                                                      ioProcessor (vts, std::move (latencyChangedCallback)),
                                                                                      presetManager (presetMgr)
{
    actionHelper = std::make_unique<ProcessorChainActionHelper> (*this);
    stateHelper = std::make_unique<ProcessorChainStateHelper> (*this);
    portMagsHelper = std::make_unique<ProcessorChainPortMagnitudesHelper> (*this);
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

    inputProcessor.prepareProcessing (osSampleRate, osSamplesPerBlock);
    outputProcessor.prepareProcessing (osSampleRate, osSamplesPerBlock);

    for (auto* processor : procs)
        processor->prepareProcessing (osSampleRate, osSamplesPerBlock);
}

void ProcessorChain::prepare (double sampleRate, int samplesPerBlock)
{
    mySampleRate = sampleRate;
    mySamplesPerBlock = samplesPerBlock;

    inputBuffer.setSize (2, samplesPerBlock * 16); // allocate extra space for upsampled buffers

    ioProcessor.prepare (sampleRate, samplesPerBlock);

    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
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

    if (proc == &outputProcessor) // we've reached the output processor, so we're done!
    {
        proc->processAudioBlock (buffer);
        outProcessed = true;
        return;
    }

    if (numOutputs == 0) // this processor has no outputs, so after we process, we're done!
    {
        proc->processAudioBlock (buffer);
        return;
    }

    if (nextNumProcs == 0) // the output of this processor is connected to nothing, so let's not waste our processing...
    {
        if (proc == &inputProcessor)
            inputProcessor.resetLevels();

        return;
    }

    proc->processAudioBlock (buffer);

    auto processBuffer = [&] (BaseProcessor* nextProc, int inputIndex, AudioBuffer<float>& nextBuffer)
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
            auto& copyNextBuffer = nextProc->getInputBuffer (inputIndex);
            copyNextBuffer.makeCopyOf (nextBuffer, true);

            nextProc->incrementNumInputsReady();
            if (nextProc->getNumInputsReady() < nextProc->getNumInputConnections())
                return; // not all the inputs are ready yet...

            runProcessor (nextProc, copyNextBuffer, outProcessed);
            nextProc->clearNumInputsReady();
        }
    };

    for (int i = 0; i < numOutputs; ++i)
    {
        auto* outBuffer = proc->getOutputBuffer (i);
        if (outBuffer == nullptr)
            outBuffer = &buffer;

        const int numOutProcs = proc->getNumOutputConnections (i);
        for (int j = numOutProcs - 1; j >= 0; --j)
        {
            const auto& connectionInfo = proc->getOutputConnection (i, j);
            processBuffer (connectionInfo.endProc, connectionInfo.endPort, *outBuffer);

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

    // prepare port magnitudes
    portMagsHelper->preparePortMagnitudes();

    const auto osNumSamples = (int) osBlock.getNumSamples();
    const auto inputNumChannels = (int) osBlock.getNumChannels();

    // process mono or stereo buffer?
    {
        inputBuffer.setSize (inputNumChannels, osNumSamples, false, false, true);
        inputBuffer.clear();

        for (int ch = 0; ch < inputNumChannels; ++ch)
            inputBuffer.copyFrom (ch, 0, osBlock.getChannelPointer ((size_t) ch), osNumSamples);
    }

    // run processing chain
    bool outProcessed = false;
    runProcessor (&inputProcessor, inputBuffer, outProcessed);

    if (! outProcessed)
        outputProcessor.resetLevels();

    // do output processing (downsampling, output gain)
    if (auto* outBuffer = outputProcessor.getOutputBuffer())
        ioProcessor.processAudioOutput (*outBuffer, buffer, outProcessed);
}

void ProcessorChain::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
    jassert (presetManager != nullptr);

    if (! presetManager->getIsDirty())
        presetManager->setIsDirty (true);
}
