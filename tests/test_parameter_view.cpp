#include <catch2/catch_test_macros.hpp>

#include "dsp/parameter/parameter_view.h"

using namespace ml::dsp::parameter;

TEST_CASE("ParameterView view binds to parameter", "[parameterview][binding]")
{
    auto param = Parameter::makeInteger("TestParam", 5, {0, 10});
    auto view = IntegerParameterView{};
    view.referTo(param);
    
    SECTION("ParameterView refers to param after referTo") {
        REQUIRE(view.get() == 5);
    }

    SECTION("Mutation of Parameter reflects in ParameterView referring to it") {
        param.set(3);
        REQUIRE(view.get() == 3);
    }

    SECTION("Mutation of ParameterView reflects in Parameter that it refers to") {
        view.set(10);
        REQUIRE(param.get<std::int32_t>() == 10);
    }

    SECTION("If setting ParameterView fails it, it does not get applied to referred Parameter") {
        REQUIRE_FALSE(view.set(-1));
        REQUIRE_FALSE(param.get<std::int32_t>() == -1);
    }
}