#pragma once

#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace test_utils {
    inline bool floatEq(float a, float b, float epsilon = 1e-5f) noexcept
    {
        return std::abs(a - b) < epsilon;
    }
}