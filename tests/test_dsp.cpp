#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "dsp/dsp.h"

using namespace ml::dsp;

using Catch::Matchers::WithinAbs;

TEST_CASE("Decibel to linear conversion is correct", "[dsp][dbtolinear]")
{
    REQUIRE_THAT(dBtoLinear(0.0f), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(dBtoLinear(-6.0f), WithinAbs(0.501187f, 0.0001f));
    REQUIRE_THAT(dBtoLinear(-60.0f), WithinAbs(0.001f, 0.0001f));
}

TEST_CASE("Equal power pan gains are correct", "[dsp][equalpowerpan]")
{
    auto [leftGain, rightGain] = equalPowerPanGains(-1.0f);
    REQUIRE_THAT(leftGain, WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(rightGain, WithinAbs(0.0f, 0.0001f));

    std::tie(leftGain, rightGain) = equalPowerPanGains(0.0f);
    REQUIRE_THAT(leftGain, WithinAbs(0.7071f, 0.0001f));
    REQUIRE_THAT(rightGain, WithinAbs(0.7071f, 0.0001f));

    std::tie(leftGain, rightGain) = equalPowerPanGains(1.0f);
    REQUIRE_THAT(leftGain, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rightGain, WithinAbs(1.0f, 0.0001f));
}