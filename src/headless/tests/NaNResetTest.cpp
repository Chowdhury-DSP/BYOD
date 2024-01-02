#include "UnitTests.h"

class NaNResetTest : public UnitTest
{
public:
    NaNResetTest() : UnitTest ("NaN Reset Test")
    {
    }

    void runTest() override
    {
        beginTest ("NaN Reset Test");

        static constexpr int blockSize = 512;

        BYOD byod;
        byod.prepareToPlay (48000.0, blockSize);

        MidiBuffer midi;
        AudioBuffer<float> buffer { 2, blockSize };

        {
            for (auto [_, data] : chowdsp::buffer_iters::channels (buffer))
                std::fill (data.begin(), data.end(), std::numeric_limits<float>::quiet_NaN());
            byod.processBlock (buffer, midi);
            const auto mag = chowdsp::BufferMath::getMagnitude (buffer);
            expectEquals (mag, 0.0f);
        }

        {
            for (auto [_, data] : chowdsp::buffer_iters::channels (buffer))
                std::fill (data.begin(), data.end(), 1.0f);
            byod.processBlock (buffer, midi);
            const auto mag = chowdsp::BufferMath::getMagnitude (buffer);
            expectGreaterOrEqual (mag, 1.0f);
        }
    }
};

static NaNResetTest nanResetTest;
