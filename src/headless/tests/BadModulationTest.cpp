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
        for (auto [name, factory] : ProcessorStore::getStoreMap())
        {
            const auto proc = factory (nullptr);

            bool hasModulationInputs = false;
            for (int i = 0; i < proc->getNumInputs(); ++i)
                hasModulationInputs |= proc->getOutputPortType(i) == PortType::modulation;

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
                        proc->getInputBuffer (portIdx).makeCopyOf (buffer, true);

                    proc->processAudioBlock (buffer);

                    for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
                    {
                        const auto magnitude = proc->getOutputBuffer (portIdx)->getMagnitude (0, blockSize);
                        const auto isValid = ! std::isnan (magnitude) && ! std::isinf (magnitude) && std::abs (magnitude) < 50.0f;
                        expect (isValid, "Modulation output is invalid!");
                    }
                }
            },
            processorsWithNoModulation);
    }
};

static BadModulationTest badModulationTest;
