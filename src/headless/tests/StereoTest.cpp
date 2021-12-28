#include "UnitTests.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 8192;
} // namespace

class StereoTest : public UnitTest
{
public:
    StereoTest() : UnitTest ("Stereo Test")
    {
    }

    void monoInputTest()
    {
        BYOD plugin;
        auto* undoManager = plugin.getVTS().undoManager;
        auto& chain = plugin.getProcChain();
        auto& actionHelper = chain.getActionHelper();

        plugin.prepareToPlay (sampleRate, blockSize);

        auto& splitterFactory = ProcessorStore::getStoreMap().at ("Stereo Splitter");
        auto& mergerFactory = ProcessorStore::getStoreMap().at ("Stereo Merger");

        actionHelper.addProcessor (splitterFactory (undoManager));
        actionHelper.addProcessor (mergerFactory (undoManager));

        auto* input = &chain.getInputProcessor();
        auto* splitter = chain.getProcessors()[0];
        auto* merger = chain.getProcessors()[1];
        auto* output = &chain.getOutputProcessor();

        actionHelper.removeConnection ({ input, 0, output, 0 });
        actionHelper.addConnection ({ input, 0, splitter, 0 });
        actionHelper.addConnection ({ splitter, 0, merger, 0 });
        actionHelper.addConnection ({ splitter, 1, merger, 1 });
        actionHelper.addConnection ({ merger, 0, output, 0 });

        // set to mono
        input->getVTS().getParameter ("mono_stereo")->setValueNotifyingHost (0.0f);

        AudioBuffer<float> buffer (2, blockSize);
        chain.processAudio (buffer);

        FloatVectorOperations::fill (buffer.getWritePointer (0), 1.0f, blockSize);
        FloatVectorOperations::fill (buffer.getWritePointer (1), 1.0f, blockSize);
        chain.processAudio (buffer);

        auto leftMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (0) + blockSize / 2, blockSize / 2);
        auto rightMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (1) + blockSize / 2, blockSize / 2);

        expectGreaterThan (leftMinMax.getStart(), 0.4f, "Left channel minimum should be > 0!");
        expectGreaterThan (rightMinMax.getStart(), 0.4f, "Right channel minimum should be > 0!");
    }

    void stereoInputTest (bool inOrder)
    {
        BYOD plugin;
        auto* undoManager = plugin.getVTS().undoManager;
        auto& chain = plugin.getProcChain();
        auto& actionHelper = chain.getActionHelper();

        plugin.prepareToPlay (sampleRate, blockSize);

        auto& splitterFactory = ProcessorStore::getStoreMap().at ("Stereo Splitter");
        auto& mergerFactory = ProcessorStore::getStoreMap().at ("Stereo Merger");

        actionHelper.addProcessor (splitterFactory (undoManager));
        actionHelper.addProcessor (mergerFactory (undoManager));

        auto* input = &chain.getInputProcessor();
        auto* splitter = chain.getProcessors()[0];
        auto* merger = chain.getProcessors()[1];
        auto* output = &chain.getOutputProcessor();

        actionHelper.removeConnection ({ input, 0, output, 0 });
        actionHelper.addConnection ({ input, 0, splitter, 0 });

        if (inOrder)
        {
            actionHelper.addConnection ({ splitter, 0, merger, 0 });
            actionHelper.addConnection ({ splitter, 1, merger, 1 });
        }
        else
        {
            actionHelper.addConnection ({ splitter, 1, merger, 1 });
            actionHelper.addConnection ({ splitter, 0, merger, 0 });
        }

        actionHelper.addConnection ({ merger, 0, output, 0 });

        // set to stereo
        input->getVTS().getParameter ("mono_stereo")->setValueNotifyingHost (0.35f);

        AudioBuffer<float> buffer (2, blockSize);
        chain.processAudio (buffer);

        FloatVectorOperations::fill (buffer.getWritePointer (0), 1.0f, blockSize);
        FloatVectorOperations::fill (buffer.getWritePointer (1), -1.0f, blockSize);
        chain.processAudio (buffer);

        auto leftMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (0) + blockSize / 2, blockSize / 2);
        auto rightMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (1) + blockSize / 2, blockSize / 2);

        expectGreaterThan (leftMinMax.getStart(), 0.4f, "Left channel minimum should be > 0!");
        expectLessThan (rightMinMax.getStart(), -0.4f, "Right channel minimum should be < 0!");
    }

    void leftChannelTest()
    {
        BYOD plugin;
        auto* undoManager = plugin.getVTS().undoManager;
        auto& chain = plugin.getProcChain();
        auto& actionHelper = chain.getActionHelper();

        plugin.prepareToPlay (sampleRate, blockSize);

        auto& splitterFactory = ProcessorStore::getStoreMap().at ("Stereo Splitter");
        actionHelper.addProcessor (splitterFactory (undoManager));

        auto* input = &chain.getInputProcessor();
        auto* splitter = chain.getProcessors()[0];
        auto* output = &chain.getOutputProcessor();

        actionHelper.removeConnection ({ input, 0, output, 0 });
        actionHelper.addConnection ({ input, 0, splitter, 0 });
        actionHelper.addConnection ({ splitter, 0, output, 0 });

        // set to stereo
        input->getVTS().getParameter ("mono_stereo")->setValueNotifyingHost (0.35f);

        AudioBuffer<float> buffer (2, blockSize);
        chain.processAudio (buffer);

        FloatVectorOperations::fill (buffer.getWritePointer (0), 1.0f, blockSize);
        FloatVectorOperations::fill (buffer.getWritePointer (1), -1.0f, blockSize);
        chain.processAudio (buffer);

        auto leftMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (0) + blockSize / 2, blockSize / 2);
        auto rightMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (1) + blockSize / 2, blockSize / 2);

        expectGreaterThan (leftMinMax.getStart(), 0.4f, "Left channel minimum should be > 0!");
        expectGreaterThan (rightMinMax.getStart(), 0.4f, "Right channel minimum should be > 0!");
    }

    void rightChannelTest()
    {
        BYOD plugin;
        auto* undoManager = plugin.getVTS().undoManager;
        auto& chain = plugin.getProcChain();
        auto& actionHelper = chain.getActionHelper();

        plugin.prepareToPlay (sampleRate, blockSize);

        auto& splitterFactory = ProcessorStore::getStoreMap().at ("Stereo Splitter");
        actionHelper.addProcessor (splitterFactory (undoManager));

        auto* input = &chain.getInputProcessor();
        auto* splitter = chain.getProcessors()[0];
        auto* output = &chain.getOutputProcessor();

        actionHelper.removeConnection ({ input, 0, output, 0 });
        actionHelper.addConnection ({ input, 0, splitter, 0 });
        actionHelper.addConnection ({ splitter, 1, output, 0 });

        // set to stereo
        input->getVTS().getParameter ("mono_stereo")->setValueNotifyingHost (0.35f);

        AudioBuffer<float> buffer (2, blockSize);
        chain.processAudio (buffer);

        FloatVectorOperations::fill (buffer.getWritePointer (0), 1.0f, blockSize);
        FloatVectorOperations::fill (buffer.getWritePointer (1), -1.0f, blockSize);
        chain.processAudio (buffer);

        auto leftMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (0) + blockSize / 2, blockSize / 2);
        auto rightMinMax = FloatVectorOperations::findMinAndMax (buffer.getReadPointer (1) + blockSize / 2, blockSize / 2);

        expectLessThan (leftMinMax.getStart(), -0.4f, "Left channel minimum should be < 0!");
        expectLessThan (rightMinMax.getStart(), -0.4f, "Right channel minimum should be < 0!");
    }

    void runTest() override
    {
        beginTest ("Mono Input Test");
        monoInputTest();

        beginTest ("Stereo Input Test");
        stereoInputTest (true);
        stereoInputTest (false);

        beginTest ("Left Channel Test");
        leftChannelTest();

        beginTest ("Right Channel Test");
        rightChannelTest();
    }
};

static StereoTest stereoTest;
