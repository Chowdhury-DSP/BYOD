#include "UnitTests.h"

class BadModulationTest : public UnitTest
{
public:
    BadModulationTest() : UnitTest ("Bad Modulation Test")
    {
    }

    void runTest() override
    {
        StringArray processorsWithNoModulation {};
        for (auto [name, storeEntry] : ProcessorStore::getStoreMap())
        {
            const auto proc = storeEntry.factory (nullptr);

            bool hasModulationInputs = false;
            for (int i = 0; i < proc->getNumInputs(); ++i)
                hasModulationInputs |= proc->getInputPortType (i) == PortType::modulation;

            if (! hasModulationInputs)
                processorsWithNoModulation.add (proc->getName());
        }

        runTestForAllProcessors (
            this,
            [this] (BaseProcessor* proc)
            {
                static constexpr double sampleRate = 48000.0;
                static constexpr int blockSize = 512;

                proc->prepareProcessing (sampleRate, blockSize);
                DSPArena arena {};
                arena.get_memory_resource() = ProcessorChain::allocArena (1 << 18);

                chowdsp::SineWave<float> sine;
                sine.prepare ({ sampleRate, (uint32_t) blockSize, 1 });
                sine.setFrequency (20.0f);

                juce::AudioBuffer<float> buffer { 1, blockSize };

                InputProcessor input;
                for (int portIdx = 0; portIdx < proc->getNumInputs(); ++portIdx)
                    input.addConnection (ConnectionInfo { .startProc = &input,
                                                          .startPort = 0,
                                                          .endProc = proc,
                                                          .endPort = portIdx });

                for (int i = 0; i < 10; ++i)
                {
                    buffer.clear();
                    sine.processBlock (buffer);
                    buffer.applyGain (4.0f);

                    for (int portIdx = 0; portIdx < proc->getNumInputs(); ++portIdx)
                    {
                        auto bufferView = arena.alloc_buffer (buffer);
                        chowdsp::BufferMath::copyBufferData (buffer, bufferView);
                        proc->getInputBufferView (portIdx) = bufferView;
                        jassert (chowdsp::BufferMath::sanitizeBuffer (bufferView));
                    }

                    proc->arena = &arena;
                    proc->processAudioBlock (buffer);

                    for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
                    {
                        const auto magnitude = chowdsp::BufferMath::getMagnitude (proc->getOutputBuffer (portIdx));
                        const auto isValid = ! std::isnan (magnitude) && ! std::isinf (magnitude) && std::abs (magnitude) < 50.0f;
                        expect (isValid, "Modulation output is invalid!");
                    }

                    arena.clear();
                }

                ProcessorChain::deallocArena (arena.get_memory_resource());
            },
            processorsWithNoModulation);
    }
};

static BadModulationTest badModulationTest;
