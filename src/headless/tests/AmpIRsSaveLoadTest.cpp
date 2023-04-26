#include "processors/tone/amp_irs/AmpIRs.h"

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int bufferSize = 1024;
} // namespace

class AmpIRsSaveLoadTest : public UnitTest
{
public:
    AmpIRsSaveLoadTest() : UnitTest ("Amp IRs Save/Load Test")
    {
    }

    static void process (AudioBuffer<float>& buffer, AmpIRs& irs)
    {
        buffer.clear();

        chowdsp::SawtoothWave<float> tone;
        tone.setFrequency (100.0f);
        tone.prepare ({ sampleRate, bufferSize, 1 });

        for (auto [ch, sample, data] : chowdsp::buffer_iters::sub_blocks<bufferSize> (buffer))
        {
            chowdsp::BufferView<float> bufferData { data.data(), (int) data.size() };
            auto juceBufferData = bufferData.toAudioBuffer();
            tone.processBlock (bufferData);
            irs.processAudio (juceBufferData);
        }
    }

    void runTest() override
    {
        beginTest ("Save/Load Test");

        const auto testIRFile = []
        {
            auto rootDir = File::getSpecialLocation (File::currentExecutableFile);
            while (rootDir.getFileName() != "BYOD")
                rootDir = rootDir.getParentDirectory();
            return rootDir.getChildFile ("src/headless/tests/test_ir.wav");
        }();

        std::unique_ptr<XmlElement> state;
        AudioBuffer<float> refSignal { 1, (int) sampleRate };

        {
            AmpIRs irs;
            irs.loadIRFromStream (testIRFile.createInputStream());
            irs.prepare (sampleRate, bufferSize);
            MessageManager::getInstance()->runDispatchLoopUntil (50);
            process (refSignal, irs);
            state = irs.toXML();
        }

        using namespace chowdsp::version_literals;

        AudioBuffer<float> testSignal { 1, (int) sampleRate };
        {
            AmpIRs irs;
            irs.fromXML (state.get(), "1.1.4"_v, false);
            irs.prepare (sampleRate, bufferSize);
            MessageManager::getInstance()->runDispatchLoopUntil (50);
            process (testSignal, irs);
        }

        const auto* refData = refSignal.getReadPointer (0);
        const auto* testData = testSignal.getReadPointer (0);
        for (int i = 1000; i < (int) sampleRate; ++i)
            expectWithinAbsoluteError (testData[i], refData[i], 0.75f);
    }
};

static AmpIRsSaveLoadTest ampIRsSaveLoadTest;
