#include <catch2/catch_test_macros.hpp>

#include "dsp/processors/processor_chain.h"

using namespace ml::dsp::processors;

struct TestProcessor
{
    int incr = 0.0f;
    static inline int counter = 0;

    void prepare(float sampleRate)
    {
        incr = static_cast<int>(sampleRate);
    }

    void process(float* const*, unsigned int)
    {
        counter += incr;
    }
};

TEST_CASE("ProcessorChain calls prepare and process of its processors", "[processorchain]")
{
    ProcessorChain<TestProcessor, TestProcessor> chain;
    chain.prepare(44100.0f);

    SECTION("ProcessorChain calls prepare of its processors") {
        REQUIRE(chain.getProcessor<0>().incr == 44100);
        REQUIRE(chain.getProcessor<1>().incr == 44100);
    }

    SECTION("ProcessorChain calls process of its processors") {
        chain.process(nullptr, 0);
        REQUIRE(TestProcessor::counter == 88200);
    }
}