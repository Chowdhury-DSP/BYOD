#include "UnitTests.h"

class PreBufferTest : public UnitTest
{
public:
    PreBufferTest() : UnitTest ("Pre-Buffer Test")
    {
    }

    void runTest() override
    {
        doForAllProcessors ([] (BaseProcessor* proc) {
            std::cout << "Testing: " << proc->getName() << std::endl;
        });
    }
};

static PreBufferTest preBufferTest;
