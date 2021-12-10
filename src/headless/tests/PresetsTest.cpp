#include "UnitTests.h"

namespace
{
constexpr double sampleRateToUse = 48000.0;
constexpr int blockSize = 480;
} // namespace

class PresetsTest : public UnitTest
{
public:
    PresetsTest() : UnitTest ("Presets Test")
    {
    }

    void presetsTest()
    {
        AudioBuffer<float> buffer (2, blockSize);
        auto rand = getRandom();
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int n = 0; n < blockSize; ++n)
                buffer.setSample (ch, n, rand.nextFloat() * 2.0f - 1.0f);
        }
        MidiBuffer midi;

        BYOD plugin;
        plugin.prepareToPlay (sampleRateToUse, blockSize);

        std::atomic_bool finished = false;
        WaitableEvent waiter;
        int bufferCount = 0;

        auto async = std::async (std::launch::async, [&]
                                 {
                                     while (! finished.load())
                                     {
                                         plugin.processBlock (buffer, midi);
                                         bufferCount++;
                                     }
                                     waiter.signal();
                                 });

        int numPrograms = plugin.getNumPrograms();
        for (int i = 0; i < numPrograms; ++i)
        {
            std::cout << "Loading preset: " << plugin.getProgramName (i) << std::endl;
            plugin.setCurrentProgram (i);
            MessageManager::getInstance()->runDispatchLoopUntil (50);
            expectEquals (plugin.getCurrentProgram(), i, "Current preset index is incorrect!");
        }

        finished.store (true);
        waiter.wait();
        std::cout << "Processed " << bufferCount << " buffers while loading presets" << std::endl;
    }

    void runTest() override
    {
        beginTest ("Presets Test");
        presetsTest();
    }
};

static PresetsTest presetsTest;
