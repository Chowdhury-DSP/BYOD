#include "processors/drive/waveshaper/Waveshaper.h"

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int bufferSize = 1024;
} // namespace

using namespace SurgeWaveshapers;

class WaveshaperTest : public UnitTest
{
public:
    WaveshaperTest() : UnitTest ("Waveshaper Test")
    {
    }

    void bufferTest (int numChannels, int shapeIndex)
    {
        Waveshaper waveshaper;
        waveshaper.getVTS().getParameter ("shape")->setValueNotifyingHost ((float) shapeIndex / (float) (n_ws_types - 1));
        waveshaper.getVTS().getParameter ("drive")->setValueNotifyingHost (1.0f);
        waveshaper.prepare (sampleRate, bufferSize);

        AudioBuffer<float> buffer (numChannels, bufferSize);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < bufferSize; ++n)
                buffer.setSample (ch, n, rand.nextFloat() * 2.0f - 1.0f);
        }

        waveshaper.processAudio (buffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int n = 0; n < bufferSize; ++n)
            {
                auto x = buffer.getSample (ch, n);
                expect (! std::isnan (x), "NAN found in buffer!");
                expect (! std::isinf (x), "INF found in buffer!");
                expect (std::abs (x) < 100.0f, "Too large value found!");
            }
        }
    }

    void runTest() override
    {
        rand = getRandom();

        for (int shapeIdx = 0; shapeIdx < ws_type::n_ws_types; ++shapeIdx)
        {
            beginTest (String (wst_names[shapeIdx]));
            bufferTest (1, shapeIdx);
            bufferTest (2, shapeIdx);
        }
    }

private:
    Random rand;
};

static WaveshaperTest waveshaperTest;
