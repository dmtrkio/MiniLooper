#pragma once

#include <algorithm>
#include <cmath>
#include <utility>
#include <numbers>

namespace dsp {
    constexpr float linearToDb(const float value, const float minDb = -100.0f)
    {
        if (value <= 0.0000001f)
            return minDb;

        return 20.0f * std::log10(value);
    }

    inline float dBtoLinear(const float value) noexcept
    {
        static constexpr float scale = 1.f / 20.f;
        return std::pow<float>(10.0f, value * scale);
    }

    inline std::tuple<float, float> equalPowerPanGains(float pan)
    {
        pan = std::clamp(pan, -1.0f, 1.0f);
        const auto angle = (pan + 1.0f) * 0.25f * std::numbers::pi_v<float>;
        const auto leftGain = std::cos(angle);
        const auto rightGain = std::sin(angle);
        return {leftGain, rightGain};
    }

    template<typename T>
    struct Range
    {
        T min; // inclusive
        T max; // inclusive

        [[nodiscard]] T clamp(T value) const { return std::clamp(value, min, max); }
        [[nodiscard]] bool contains(T value) const { return value >= min && value <= max; }
    };

    class Rms
    {
    public:
        void prepare(const float windowSizeSamples)
        {
            assert(windowSizeSamples > 0.0f);

            alpha_ = std::exp(-1.0f / windowSizeSamples);
            state_ = 0.0f;
        }

        void updateSum(const float sample)
        {
            const float squared = sample * sample;
            state_ = alpha_ * state_ + (1.0f - alpha_) * squared;
        }

        [[nodiscard]] float getRms() const noexcept
        {
            return std::sqrt(std::max(state_, 0.0f));;
        }

        float operator()(const float sample)
        {
            updateSum(sample);
            return getRms();
        }

        float operator()(const float* samples, const std::size_t nSamples)
        {
            for (auto i{0u}; i < nSamples; ++i)
                updateSum(samples[i]);
            return getRms();
        }

    private:
        float alpha_{0.0f};
        float state_{0.0f};
    };

    class BallisticsFilter
    {
    public:
        void prepare(float sampleRate, float attackMs, float releaseMs, float initial)
        {
            attack_  = std::exp(-1.0f / (attackMs  * sampleRate / 1000.0f));
            release_ = std::exp(-1.0f / (releaseMs * sampleRate / 1000.0f));
            state_ = initial;
        }

        [[nodiscard]] float getState() const noexcept
        {
            return state_;
        }

        float operator()(float x)
        {
            const auto coeff = (x > state_) ? attack_ : release_;
            state_ = coeff * state_ + (1.0f - coeff) * x;
            return state_;
        }

    private:
        float attack_{0.0f};
        float release_{0.0f};
        float state_{0.0f};
    };

    template<typename T>
    class Smoother {
    public:
        void setTarget(T newTarget) noexcept { target_ = newTarget; }
        void init(T initial) noexcept { setTarget(initial); snap(); }
        void snap() { current_ = target_; }

        void setSmoothingTime(T sampleRate, T timeInSeconds) noexcept
        {
            setSmoothingFrames(sampleRate * timeInSeconds);
        }

        void setSmoothingFrames(T timeInFrames) noexcept
        {
            coeff_ = std::exp(T(-1) / timeInFrames);
        }

        T operator()()
        {
            current_ = current_ * coeff_ + target_ * (T(1) - coeff_);
            return current_;
        }

        Smoother& operator=(T newTarget) noexcept
        {
            setTarget(newTarget);
            return *this;
        }

        T getCurrent() const noexcept { return current_; }
        T getTarget() const noexcept { return target_; }

    private:
        T current_;
        T target_;
        T coeff_;
    };

    using FloatSmoother = Smoother<float>;
}
