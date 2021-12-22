#include "UnitTests.h"

namespace
{
constexpr double testSampleRate = 48000.0;
constexpr int testBlockSize = 1024;
} // namespace

class SilenceTest : public UnitTest
{
public:
    SilenceTest() : UnitTest ("Silence Test")
    {
    }

    void testBuffer (const float* buffer)
    {
        auto minMax = FloatVectorOperations::findMinAndMax (buffer, testBlockSize);
        auto max = jmax (std::abs (minMax.getStart()), std::abs (minMax.getEnd()));

        expectGreaterThan (max, 1.0e-6f, "Max output value is too quiet!");
    }

    void runTest() override
    {
        auto r = getRandom();
        runTestForAllProcessors (this,
                                 [&] (BaseProcessor* proc)
                                 {
                                     if (proc->getNumInputs() != 1)
                                         return;

                                     proc->prepare (testSampleRate, testBlockSize);

                                     AudioBuffer<float> buffer (1, testBlockSize);
                                     for (int n = 0; n < testBlockSize; ++n)
                                         buffer.setSample (0, n, r.nextFloat() * 2.0f - 1.0f);

                                     proc->processAudio (buffer);

                                     testBuffer (buffer.getReadPointer (0));
                                 });
    }
};

static SilenceTest silenceTest;
