#pragma once

#include <cmath>

namespace ml::dsp::filter {
    class BallisticsFilter
    {
    public:
        void prepare(const float sampleRate, const float attackMs, const float releaseMs, const float initial)
        {
            attack_  = std::exp(-1.0f / (attackMs  * sampleRate / 1000.0f));
            release_ = std::exp(-1.0f / (releaseMs * sampleRate / 1000.0f));
            state_ = initial;
        }

        [[nodiscard]] float getState() const noexcept
        {
            return state_;
        }

        float operator()(const float x)
        {
            const auto coefficient = (x > state_) ? attack_ : release_;
            state_ = coefficient * state_ + (1.0f - coefficient) * x;
            return state_;
        }

    private:
        float attack_{0.0f};
        float release_{0.0f};
        float state_{0.0f};
    };
}