#include "ProcessorChain.h"
#include "../ParameterHelpers.h"
#include "ProcessorChainActionHelper.h"
#include "ProcessorChainStateHelper.h"

namespace
{
const String oversamplingTag = "oversampling";
const String inGainTag = "in_gain";
const String outGainTag = "out_gain";
const String dryWetTag = "dry_wet";

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
                                                                                      presetManager (presetMgr)
{
    actionHelper = std::make_unique<ProcessorChainActionHelper> (*this);
    stateHelper = std::make_unique<ProcessorChainStateHelper> (*this);

    using namespace std::placeholders;
    procStore.addProcessorCallback = std::bind (&ProcessorChainActionHelper::addProcessor, actionHelper.get(), _1);
    procStore.replaceProcessorCallback = std::bind (&ProcessorChainActionHelper::replaceProcessor, actionHelper.get(), _1, _2);

    oversamplingParam = vts.getRawParameterValue (oversamplingTag);
    inGainParam = vts.getRawParameterValue (inGainTag);
    outGainParam = vts.getRawParameterValue (outGainTag);
    dryWetParam = vts.getRawParameterValue (dryWetTag);

    for (int i = 0; i < 5; ++i)
        overSample[i] = std::make_unique<dsp::Oversampling<float>> (2, i, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
}

ProcessorChain::~ProcessorChain() = default;

void ProcessorChain::createParameters (Parameters& params)
{
    params.push_back (std::make_unique<AudioParameterChoice> (oversamplingTag,
                                                              "Oversampling",
                                                              StringArray { "1x", "2x", "4x", "8x", "16x" },
                                                              1));

    using namespace ParameterHelpers;
    createGainDBParameter (params, inGainTag, "In Gain", -72.0f, 18.0f, 0.0f);
    createGainDBParameter (params, outGainTag, "Out Gain", -72.0f, 18.0f, 0.0f);
    createPercentParameter (params, dryWetTag, "Dry/Wet", 1.0f);
}

void ProcessorChain::initializeProcessors (int curOS)
{
    const auto osFactor = (int) overSample[curOS]->getOversamplingFactor();
    const double osSampleRate = mySampleRate * osFactor;
    const int osSamplesPerBlock = mySamplesPerBlock * osFactor;

    auto prepProcessor = [=] (BaseProcessor& proc) {
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

    int curOS = static_cast<int> (*oversamplingParam);
    prevOS = curOS;
    for (int i = 0; i < 5; ++i)
        overSample[i]->initProcessing ((size_t) samplesPerBlock);

    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };
    for (auto* gain : { &inGain, &outGain })
    {
        gain->prepare (spec);
        gain->setRampDurationSeconds (0.1);
    }

    dryWetMixer.prepare (spec);

    initializeProcessors (curOS);
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

    auto processBuffer = [&] (BaseProcessor* nextProc, AudioBuffer<float>& nextBuffer) {
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

void ProcessorChain::processAudio (AudioBuffer<float> buffer)
{
    SpinLock::ScopedTryLockType tryProcessingLock (processingLock);
    if (! tryProcessingLock.isLocked())
        return;

    // oversampling
    int curOS = static_cast<int> (*oversamplingParam);
    if (prevOS != curOS)
    {
        initializeProcessors (curOS);
        prevOS = curOS;
    }

    // set input, output, and dry/wet
    inGain.setGainDecibels (inGainParam->load());
    outGain.setGainDecibels (outGainParam->load());
    dryWetMixer.setDryWet (dryWetParam->load());

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);
    inGain.process (context);
    dryWetMixer.copyDryBuffer (buffer);

    auto osBlock = overSample[curOS]->processSamplesUp (block);

    // process mono or stereo buffer?
    const auto osNumSamples = (int) osBlock.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    inputBuffer.setSize (2, osNumSamples, false, false, true);
    inputBuffer.clear();

    for (int ch = 0; ch < numChannels; ++ch)
        inputBuffer.copyFrom (ch, 0, osBlock.getChannelPointer ((size_t) ch), osNumSamples);

    bool outProcessed = false;
    runProcessor (&inputProcessor, inputBuffer, outProcessed);

    if (outProcessed)
    {
        auto& outBuffer = *outputProcessor.getOutputBuffer();
        for (int ch = 0; ch < numChannels; ++ch)
            FloatVectorOperations::copy (osBlock.getChannelPointer ((size_t) ch),
                                         outBuffer.getReadPointer (ch),
                                         osNumSamples);
    }
    else
    {
        osBlock.clear();
    }

    overSample[curOS]->processSamplesDown (block);

    auto latencySamples = overSample[curOS]->getLatencyInSamples();
    dryWetMixer.processBlock (buffer, latencySamples);
    outGain.process (context);
}

void ProcessorChain::parameterChanged (const juce::String& /*parameterID*/, float /*newValue*/)
{
    jassert (presetManager != nullptr);

    if (! presetManager->getIsDirty())
        presetManager->setIsDirty (true);
}
