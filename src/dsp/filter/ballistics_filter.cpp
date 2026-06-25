#include "ballistics_filter.h"

#include <cmath>

namespace ml::dsp::filter {
    void BallisticsFilter::prepare(const float sampleRate, const float attackMs, const float releaseMs, const float initial) noexcept
    {
        const auto k = sampleRate / 1000.0f;
        attack_  = std::exp(-1.0f / (attackMs  * k));
        release_ = std::exp(-1.0f / (releaseMs * k));
        state_ = initial;
    }

    float BallisticsFilter::process(const float x) noexcept
    {
        const auto coefficient = (x > state_) ? attack_ : release_;
        state_ = coefficient * state_ + (1.0f - coefficient) * x;
        return state_;
    }

    float BallisticsFilter::getState() const noexcept
    {
        return state_;
    }
}