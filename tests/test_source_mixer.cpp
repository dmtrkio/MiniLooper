#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "looper/source_mixer.h"

using namespace ml::looper;

using Catch::Matchers::WithinAbs;

TEST_CASE("SourceChannel parameter tree contains expected parameters", "[sourcechannel][parametertree]")
{
    SourceChannel channel("TestChannel");
    const auto paramTree = channel.getParameterTree();

    REQUIRE(paramTree.isValid());
    REQUIRE(paramTree.getName() == "TestChannel");

    using ml::dsp::parameter::ParameterType;

    SECTION("Input1 parameters are present and have correct types") {
        const auto input1Tree = paramTree["Input1"];
        REQUIRE(input1Tree.isValid());
        REQUIRE(input1Tree["Source"].isParameter());
        REQUIRE(input1Tree["Source"].asParameterUnsafe().getType() == ParameterType::Integer);
        REQUIRE(input1Tree["GainDb"].isParameter());
        REQUIRE(input1Tree["GainDb"].asParameterUnsafe().getType() == ParameterType::Float);
    }

    SECTION("Input2 parameters are present and have correct types") {
        const auto input2Tree = paramTree["Input2"];
        REQUIRE(input2Tree.isValid());
        REQUIRE(input2Tree["Source"].isParameter());
        REQUIRE(input2Tree["Source"].asParameterUnsafe().getType() == ParameterType::Integer);
        REQUIRE(input2Tree["GainDb"].isParameter());
        REQUIRE(input2Tree["GainDb"].asParameterUnsafe().getType() == ParameterType::Float);
    }

    SECTION("Stereo parameter is present and has correct type") {
        REQUIRE(paramTree["Stereo"].isParameter());
        REQUIRE(paramTree["Stereo"].asParameterUnsafe().getType() == ParameterType::Boolean);
    }

    SECTION("Mix parameters are present") {
        const auto t = paramTree["Mix"];
        REQUIRE(t.isValid());
    }

    SECTION("FX parameters are present") {
        const auto t = paramTree["FX"];
        REQUIRE(t.isValid());
    }
}

TEST_CASE("SourceChannel correctly writes data to output buffers", "[sourcechannel][processing]")
{
    SourceChannel channel("TestChannel");
    auto paramTree = channel.getParameterTree();
    channel.skipInternalProcessing(true); // Skip internal processing for testing

    constexpr std::size_t kBufferSize = 2;
    constexpr std::size_t kNumInputs = 2;

    const float input1[kBufferSize] = {0.1f,0.2f};
    const float input2[kBufferSize] = {0.3f, 0.4f};
    const float* inputs[kNumInputs] = {input1, input2};

    float outputL[kBufferSize] = {0.0f, 0.0f};
    float outputR[kBufferSize] = {0.0f, 0.0f};
    float* outputs[2] = {outputL, outputR};

    channel.prepare(kNumInputs, kBufferSize, 44100);

    constexpr float kEpsilon = 1e-6f;

    SECTION("By default SourceChannel does not write anything") {
        channel.processAdding(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(0.0f, kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(0.0f, kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(0.0f, kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(0.0f, kEpsilon));
    }

    SECTION("SourceChannel writes inputs to left and right when set to stereo") {
        paramTree["Input1"]["Source"].asParameterUnsafe().set(0);
        paramTree["Input2"]["Source"].asParameterUnsafe().set(1);
        paramTree["Stereo"].asParameterUnsafe().set(true);

        channel.processAdding(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(input1[0], kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(input1[1], kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(input2[0], kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(input2[1], kEpsilon));

        channel.processAdding(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(input1[0] * 2.0f, kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(input1[1] * 2.0f, kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(input2[0] * 2.0f, kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(input2[1] * 2.0f, kEpsilon));
    }

    SECTION("SourceChannel sums inputs when set to mono") {
        paramTree["Input1"]["Source"].asParameterUnsafe().set(0);
        paramTree["Input2"]["Source"].asParameterUnsafe().set(1);
        paramTree["Stereo"].asParameterUnsafe().set(false);

        channel.processAdding(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(input1[0] + input2[0], kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(input1[1] + input2[1], kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(input1[0] + input2[0], kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(input1[1] + input2[1], kEpsilon));
    }

    SECTION("SourceChannel only writes those inputs that are selected") {
        paramTree["Input1"]["Source"].asParameterUnsafe().set(0);
        paramTree["Input2"]["Source"].asParameterUnsafe().set(SourceChannel::kNoInput);
        paramTree["Stereo"].asParameterUnsafe().set(false);

        channel.processAdding(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(input1[0], kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(input1[1], kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(input1[0], kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(input1[1], kEpsilon));
    }
}

TEST_CASE("SourceMixer correctly mixes SourceChannels to output buffers", "[sourcemixer][mixing]")
{
    SourceMixer mixer;
    mixer.skipInternalProcessing(true); // Skip internal processing for testing

    auto paramTree = mixer.getParameterTree();
    auto source1Tree = paramTree["SourceChannel1"];
    auto source2Tree = paramTree["SourceChannel2"];
    
    REQUIRE(source1Tree.isValid());
    REQUIRE(source2Tree.isValid());

    constexpr std::size_t kBufferSize = 2;
    constexpr std::size_t kNumInputs = 2;

    const float input1[kBufferSize] = {0.1f,0.2f};
    const float input2[kBufferSize] = {0.3f, 0.4f};
    const float* inputs[kNumInputs] = {input1, input2};

    float outputL[kBufferSize] = {0.0f, 0.0f};
    float outputR[kBufferSize] = {0.0f, 0.0f};
    float* outputs[2] = {outputL, outputR};

    mixer.prepare(kNumInputs, kBufferSize, 44100);

    constexpr float kEpsilon = 1e-6f;

    SECTION("By default SourceMixer does not write anything") {
        mixer.process(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(0.0f, kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(0.0f, kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(0.0f, kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(0.0f, kEpsilon));
    }

    SECTION("SourceMixer sums outputs of both channels") {
        source1Tree["Input1"]["Source"].asParameterUnsafe().set(0);
        source2Tree["Input1"]["Source"].asParameterUnsafe().set(1);

        mixer.process(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(input1[0] + input2[0], kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(input1[1] + input2[1], kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(input1[0] + input2[0], kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(input1[1] + input2[1], kEpsilon));
    }

    SECTION("SourceMixer only sums channels that have inputs selected") {
        source1Tree["Input1"]["Source"].asParameterUnsafe().set(0);
        source2Tree["Input1"]["Source"].asParameterUnsafe().set(SourceChannel::kNoInput);

        mixer.process(inputs, outputs, kBufferSize);

        REQUIRE_THAT(outputL[0], WithinAbs(input1[0], kEpsilon));
        REQUIRE_THAT(outputL[1], WithinAbs(input1[1], kEpsilon));

        REQUIRE_THAT(outputR[0], WithinAbs(input1[0], kEpsilon));
        REQUIRE_THAT(outputR[1], WithinAbs(input1[1], kEpsilon));
    }
}