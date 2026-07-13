#include <format>
#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"

using namespace ml::dsp::effects;
using namespace ml::dsp::parameter;

static int gCounter = 0;

class TestEffectStep : public EffectBase
{
public:
    explicit TestEffectStep(const std::string& name, int numberToAdd)
        : EffectBase(name)
        , number(numberToAdd)
    {
        attachParameters({ParameterTree{Parameter::makeInteger("Number", number, {INT_MIN, INT_MAX})}});
    }

    int number;

protected:
    void prepareInner(float) override
    {
        gCounter += number;
    }

    void processInner(float *const *, unsigned int) noexcept override
    {
        gCounter += number;
    }

    void resetInner() noexcept override
    {
        gCounter += number;
    }
};

static std::unique_ptr<EffectBase> makeTestEffectStep(int numberToAdd)
{
    return std::make_unique<TestEffectStep>(std::format("Step {}", numberToAdd), numberToAdd);
}

class TestEffectMultistep : public TestEffectStep 
{
public:
    TestEffectMultistep(const std::vector<int>& numbers) : TestEffectStep("Test", 0)
    {
        gCounter = 0;
        for (const int& n : numbers) {
            addProcessingStep(makeTestEffectStep(n)).setEnabled(true);
        }
        setEnabled(true);
    }
};

TEST_CASE("EffectBase internal processing chain works", "[EffectBase][internalchain]")
{
    const std::vector<int> testNumbers = {12, 55, 4400, 939, 479};
    const int testSum = std::ranges::fold_left(testNumbers, 0, std::plus<>());

    auto testEffect = TestEffectMultistep(testNumbers);

    SECTION("prepare is called for each step") {
        gCounter = 0;
        testEffect.prepare(44100);
        REQUIRE(gCounter == testSum);
    }

    SECTION("process is called for each step") {
        gCounter = 0;
        testEffect.process(nullptr, 0);
        REQUIRE(gCounter == testSum);
    }

    SECTION("reset is called for each step") {
        gCounter = 0;
        testEffect.reset();
        REQUIRE(gCounter == testSum);
    }

    SECTION("parameter tree contains each step's internal tree") {
        const auto& paramTree = testEffect.getParameterTree();
        REQUIRE(paramTree.isValid());
        for (const int n : testNumbers) {
            const auto name = std::format("Step {}", n);
            const auto stepParamTree = paramTree[name];
            REQUIRE(stepParamTree.isSubTree());
            const auto param = stepParamTree["Number"];
            REQUIRE(param.isParameter());
            REQUIRE(param.asParameterUnsafe().get<int>() == n);
        }
    }
}