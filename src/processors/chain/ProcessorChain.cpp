#include "ProcessorChain.h"
#include "ProcessorChainActionHelper.h"
#include "ProcessorChainPortMagnitudesHelper.h"
#include "ProcessorChainStateHelper.h"
#include "processors/chain/ChainIOProcessor.h"

namespace ChainHelperFuncs
{
[[maybe_unused]] static void printBufferLevels (const AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto level = buffer.getRMSLevel (ch, 0, numSamples);
        std::cout << "Level for channel: " << ch << ": " << level << std::endl;
    }
}

static const MidiBuffer& getMidiBufferToUse (const MidiBuffer& hostMidiBuffer, MidiBuffer& internalMidiBuffer, int osFactor)
{
    if (hostMidiBuffer.isEmpty() || osFactor == 1)
        return hostMidiBuffer;

    internalMidiBuffer.clear();
    for (const auto& midiEvent : hostMidiBuffer)
        internalMidiBuffer.addEvent (midiEvent.getMessage(), midiEvent.samplePosition * osFactor);

    return internalMidiBuffer;
}
} // namespace ChainHelperFuncs

ProcessorChain::ProcessorChain (ProcessorStore& store,
                                AudioProcessorValueTreeState& vts,
                                std::unique_ptr<chowdsp::PresetManager>& presetMgr,
                                std::unique_ptr<ParamForwardManager>& paramForwarder,
                                std::function<void (int)>&& latencyChangedCallback)
    : procStore (store),
      um (vts.undoManager),
      inputProcessor (um),
      outputProcessor (um),
      ioProcessor (vts, std::move (latencyChangedCallback)),
      presetManager (presetMgr),
      paramForwardManager (paramForwarder)
{
    actionHelper = std::make_unique<ProcessorChainActionHelper> (*this);
    stateHelper = std::make_unique<ProcessorChainStateHelper> (*this, mainThreadAction);
    portMagsHelper = std::make_unique<ProcessorChainPortMagnitudesHelper> (*this);

    procs.ensureStorageAllocated (100);
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

    for (int i = procs.size() - 1; i >= 0; --i)
    {
        if (auto* proc = procs[i])
            proc->prepareProcessing (osSampleRate, osSamplesPerBlock);
    }

    deallocArena (arena.get_memory_resource());
    arena.get_memory_resource() = allocArena (getRequiredArenaSizeBytes());
}

void ProcessorChain::prepare (double sampleRate, int samplesPerBlock)
{
    mySampleRate = sampleRate;
    mySamplesPerBlock = samplesPerBlock;

    inputBuffer.setSize (2, samplesPerBlock * 16); // allocate extra space for upsampled buffers

    ioProcessor.prepare (sampleRate, samplesPerBlock);

    internalMidiBuffer.clear();
    internalMidiBuffer.ensureSize (256);

    SpinLock::ScopedLockType scopedProcessingLock (processingLock);
    initializeProcessors();
}

void ProcessorChain::reset() noexcept
{
    ioProcessor.reset();
}

void ProcessorChain::runProcessor (BaseProcessor* proc, AudioBuffer<float>& buffer, bool& outProcessed)
{
    TRACE_DSP();

    int nextNumProcs = 0;
    const int numOutputs = proc->getNumOutputs();
    int numAudioOutputs = 0;
    for (int i = 0; i < numOutputs; ++i)
    {
        const int numOutProcs = proc->getNumOutputConnections (i);
        for (int j = 0; j < numOutProcs; ++j)
            nextNumProcs += 1;

        numAudioOutputs += proc->getOutputPortType (i) == PortType::audio ? 1 : 0;
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

    if (nextNumProcs == 0 && numAudioOutputs > 0) // the output of this processor is connected to nothing, so let's not waste our processing...
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
            auto bufferView = arena.alloc_buffer (nextBuffer);
            chowdsp::BufferMath::copyBufferData (nextBuffer, bufferView);
            nextProc->getInputBufferView() = bufferView;
            auto copyNextBuffer = bufferView.toAudioBuffer();
            runProcessor (nextProc, copyNextBuffer, outProcessed);
        }
        else
        {
            auto bufferView = arena.alloc_buffer (nextBuffer);
            chowdsp::BufferMath::copyBufferData (nextBuffer, bufferView);
            nextProc->getInputBufferView (inputIndex) = bufferView;
            auto copyNextBuffer = bufferView.toAudioBuffer();

            nextProc->incrementNumInputsReady();
            if (nextProc->getNumInputsReady() < nextProc->getNumInputConnections())
                return; // not all the inputs are ready yet...

            runProcessor (nextProc, copyNextBuffer, outProcessed);
        }
    };

    for (int i = 0; i < numOutputs; ++i)
    {
        auto outBufferView = proc->getOutputBuffer (i);
        if (outBufferView.getNumSamples() == 0)
            outBufferView = buffer;
        auto outBuffer = outBufferView.toAudioBuffer();

        const int numOutProcs = proc->getNumOutputConnections (i);
        for (int j = numOutProcs - 1; j >= 0; --j)
        {
            const auto& connectionInfo = proc->getOutputConnection (i, j);
            processBuffer (connectionInfo.endProc, connectionInfo.endPort, outBuffer);

            nextNumProcs -= 1;
        }
    }
}

void ProcessorChain::processAudio (AudioBuffer<float>& buffer, const MidiBuffer& hostMidiBuffer)
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

    bool outProcessed = false;
    const auto& processMidiBuffer = ChainHelperFuncs::getMidiBufferToUse (hostMidiBuffer, internalMidiBuffer, ioProcessor.getOversamplingFactor());

    for (auto* processor : procs)
    {
        // set up MIDI buffer and arena
        processor->midiBuffer = &processMidiBuffer;
        processor->arena = &arena;

        // process standalone modulation ports
        auto noInputsConnected = processor->getNumInputConnections() == 0;
        auto modOutputConnected = processor->isOutputModulationPortConnected();
        auto onlyModOut = processor->onlyHasModulationOutput();
        if (noInputsConnected && (modOutputConnected || onlyModOut))
            runProcessor (processor, inputBuffer, outProcessed);
    }

    // run processing chain
    runProcessor (&inputProcessor, inputBuffer, outProcessed);

    for (auto* processor : procs)
    {
        processor->midiBuffer = nullptr;
        processor->clearNumInputsReady();
    }

    if (! outProcessed)
    {
        outputProcessor.resetLevels();
        inputBuffer.clear();
        ioProcessor.processAudioOutput (inputBuffer, buffer);
    }
    else
    {
        // do output processing (downsampling, output gain)
        if (auto outBuffer = outputProcessor.getOutputBuffer(); outBuffer.getNumSamples() > 0)
        {
            ioProcessor.processAudioOutput (outBuffer.toAudioBuffer(), buffer);
        }
        else
        {
            jassertfalse; // output buffer is null after output was processed?
        }
    }

    arena.clear();
}

void ProcessorChain::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
    mainThreadAction.call ([this]
                           {
                               jassert (presetManager != nullptr);

                               if (! presetManager->getIsDirty())
                                   presetManager->setIsDirty (true); },
                           true);
}

size_t ProcessorChain::getRequiredArenaSizeBytes()
{
    const auto osFactor = ioProcessor.getOversamplingFactor();
    const int osSamplesPerBlock = mySamplesPerBlock * osFactor;
    const auto osSamplesPerBlockPadded = chowdsp::Math::round_to_next_multiple (osSamplesPerBlock, 4);
    const auto bufferSizeBytes = osSamplesPerBlockPadded * 2 * sizeof (float);

    const auto numIOBuffers = chowdsp::Math::round_to_next_multiple (2 * connectionsCount + 1, 10);
    const auto ioBufferBytes = numIOBuffers * bufferSizeBytes;

    static constexpr size_t blockSize = 8192;
    const auto totalNumBytes = chowdsp::Math::round_to_next_multiple (ioBufferBytes, blockSize);

    return totalNumBytes;
}

bool ProcessorChain::needsNewArena (size_t requiredBytes) const
{
    const auto currentArenaBytes = arena.get_memory_resource().size();

    // If the current arena is too small then we need a new one
    if (currentArenaBytes < requiredBytes)
        return true;

    // If the current arena if way too big (more than 20%), then we need a new one
    if (currentArenaBytes * 4 / 5 > requiredBytes)
        return true;

    return false;
}

std::span<std::byte> ProcessorChain::allocArena (size_t bytes)
{
    juce::Logger::writeToLog ("Allocating arena of size: " + juce::String { bytes });
    return {
        (std::byte*) chowdsp::aligned_alloc (16, bytes),
        bytes,
    };
}

void ProcessorChain::deallocArena (std::span<std::byte> bytes)
{
    if (bytes.empty())
        return;

    juce::Logger::writeToLog ("De-allocating arena of size: " + juce::String { bytes.size() });
    chowdsp::aligned_free (bytes.data());
}
