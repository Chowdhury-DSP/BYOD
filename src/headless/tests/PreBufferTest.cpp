#include "UnitTests.h"

namespace
{
constexpr double testSampleRate = 48000.0;
constexpr int testBlockSize = 4096;
} // namespace

class PreBufferTest : public UnitTest
{
public:
    PreBufferTest() : UnitTest ("Pre-Buffer Test")
    {
    }

    static float getSampleAverage (const float* buffer, const int numSamples, const int startSample = 0)
    {
        float avgSample = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            avgSample += buffer[startSample + i];
        return std::abs (avgSample / float (numSamples));
    }

    void testBuffer (const float* buffer, float steadyStateMax)
    {
        auto steadyState = getSampleAverage (buffer, 512, testBlockSize - 512);
        auto start = getSampleAverage (buffer, 10);

        if (start < 1.0e-10f)
            return;

        expectLessThan (steadyState, steadyStateMax, "Steady state level is too large!");
        expectLessThan (start, juce::jmax (steadyState * 200.0f, 5.0e-5f), "DC offset present at beginning of signal!");
    }

    void runTest() override
    {
        runTestForAllProcessors (
            this,
            [=] (BaseProcessor* proc)
            {
                proc->prepareProcessing (testSampleRate, testBlockSize);

                AudioBuffer<float> buffer (1, testBlockSize);
                buffer.clear();
                proc->processAudioBlock (buffer);

                const auto steadyStateMax = [] (const auto& name)
                {
                    if (name == "Kiwi Bass Drive" || name == "Waterfall Drive" || name == "Tweed" || name == "Swinger Pre")
                        return 1.0e-4f;
                    return 1.0e-5f;
                }(proc->getName());
                testBuffer (buffer.getReadPointer (0), steadyStateMax);
            },
            StringArray { "Muff Drive", "Trumble Drive" });
    }
};

static PreBufferTest preBufferTest;
