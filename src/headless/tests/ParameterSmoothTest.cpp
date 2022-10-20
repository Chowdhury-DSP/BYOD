#include "UnitTests.h"

namespace
{
constexpr double testSampleRate = 48000.0;
constexpr int testBlockSize = 512;
} // namespace

class ParameterSmoothTest : public UnitTest
{
public:
    ParameterSmoothTest() : UnitTest ("Parameter Smooth Test")
    {
    }

    void testParameter (BaseProcessor* proc, AudioParameterFloat* param)
    {
        AudioBuffer<float> buffer (1, testBlockSize);
        buffer.clear();
        proc->processAudioBlock (buffer);

        constexpr int numParamVals = 10;
        float paramValues[numParamVals] = { 0.0f, 1.0f, 0.5f, 0.6f, 0.4f, 0.2f, 0.1f, 0.3f, 0.7f, 1.0f };
        float testData[numParamVals * testBlockSize] = { 0.0f };

        for (int i = 0; i < numParamVals; ++i)
        {
            FloatVectorOperations::fill (buffer.getWritePointer (0), 1.0f, testBlockSize);

            param->setValueNotifyingHost (paramValues[i]);
            proc->processAudioBlock (buffer);

            FloatVectorOperations::copy (&testData[i * testBlockSize], buffer.getReadPointer (0), testBlockSize);
        }

        float maxDiff = 0.0f;
        for (int i = 100; i < numParamVals * testBlockSize; ++i)
            maxDiff = jmax (maxDiff, std::abs (testData[i] - testData[i - 1]));

        expectLessThan (maxDiff, 0.1f, "Parameter is not sufficiently smooth!");
    }

    void runTest() override
    {
        runTestForAllProcessors (this,
                                 [=] (BaseProcessor* proc)
                                 {
                                     auto params = proc->getVTS().processor.getParameters();
                                     for (auto* p : params)
                                     {
                                         auto* floatParam = dynamic_cast<AudioParameterFloat*> (p);
                                         if (floatParam == nullptr) // no a float param!
                                             continue;

                                         // reset all parameters to default
                                         for (auto* dp : params)
                                             dp->setValueNotifyingHost (dp->getDefaultValue());

                                         std::cout << "  Testing parameter: " << floatParam->name << std::endl;

                                         proc->prepareProcessing (testSampleRate, testBlockSize);
                                         testParameter (proc, floatParam);
                                     }
                                 },
                                 { "Delay", "GuitarML", "Junior B", "Tweed" });
    }
};

static ParameterSmoothTest paramSmoothTest;
