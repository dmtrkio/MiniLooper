#pragma once

#include <cassert>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <numbers>

namespace ml::dsp {
    struct MinMax
    {
        float min{0.0f};
        float max{0.0f};
    };

    constexpr float linearToDb(const float value, const float minDb = -100.0f) noexcept
    {
        if (value <= 0.0000001f)
            return minDb;

        return 20.0f * std::log10(value);
    }

    inline float dBtoLinear(const float value) noexcept
    {
        static constexpr float scale = 1.f / 20.f;
        return std::pow(10.0f, value * scale);
    }

    inline std::pair<float, float> equalPowerPanGains(float pan) noexcept
    {
        pan = std::clamp(pan, -1.0f, 1.0f);
        const auto angle = (pan + 1.0f) * 0.25f * std::numbers::pi_v<float>;
        const auto leftGain = std::cos(angle);
        const auto rightGain = std::sin(angle);
        return {leftGain, rightGain};
    }
    
    inline float semitonesToPitchMultiplier(float semitones) noexcept
    {
        return std::pow(2.0f, semitones / 12.0f);
    }

    template<std::size_t N, typename F, std::size_t... Is>
    void staticFor(F&& f, std::index_sequence<Is...>)
    {
        (f(std::integral_constant<std::size_t, Is>{}), ...);
    }

    template<std::size_t N, typename F>
    void staticFor(F&& f)
    {
        staticFor<N>(std::forward<F>(f), std::make_index_sequence<N>{});
    }

    template<typename T>
    struct Range
    {
        T min; // inclusive
        T max; // inclusive

        [[nodiscard]] constexpr T clamp(T value) const noexcept { return std::clamp(value, min, max); }
        [[nodiscard]] constexpr bool contains(T value) const noexcept { return value >= min && value <= max; }
        // Linearly maps 0..1 value to min..max range, does not clamp
        [[nodiscard]] constexpr T linmap(T value) const noexcept { return std::lerp(min, max, value); }
    };

    using FloatRange = Range<float>;
    using IntRange = Range<int>;

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

    template<typename T>
    class Smoother
    {
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
            coefficient_ = std::exp(T(-1) / timeInFrames);
        }

        T operator()()
        {
            current_ = current_ * coefficient_ + target_ * (T(1) - coefficient_);
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
        T coefficient_;
    };

    template <typename T, std::size_t N>
    class MultiSmoother
    {
    public:
        using kChannelCount = std::integral_constant<std::size_t, N>;

        void setTarget(T newTarget) noexcept {
            target_ = newTarget;
        }

        void init(T initial) noexcept {
            setTarget(initial);
            snap();
        }

        void snap() noexcept {
            current_.fill(target_);
        }

        void setSmoothingTime(T sampleRate, T timeInSeconds) noexcept {
            setSmoothingFrames(sampleRate * timeInSeconds);
        }

        void setSmoothingFrames(T timeInFrames) noexcept {
            coefficient_ = std::exp(T(-1) / timeInFrames);
        }

        template <std::size_t I>
        T get() {
            static_assert(I < N, "Index out of bounds");
            current_[I] = current_[I] * coefficient_ + target_ * (T(1) - coefficient_);
            return current_[I];
        }

        MultiSmoother& operator=(T newTarget) noexcept {
            setTarget(newTarget);
            return *this;
        }

        template <std::size_t I>
        T getCurrent() const noexcept {
            static_assert(I < N, "Index out of bounds");
            return current_[I];
        }

        T getTarget() const noexcept {
            return target_;
        }

    private:
        std::array<T, N> current_;
        T target_;
        T coefficient_;
    };

    using FloatSmoother = Smoother<float>;
    using StereoFloatSmoother = MultiSmoother<float, 2>;

    inline float biasedTanh(float x, float bias, float biasTanh) noexcept
    {
        return std::tanh(x + bias) - biasTanh;
    }

    inline float biasedTanh(float x, float bias) noexcept
    {
        return biasedTanh(x, bias, std::tanh(bias));
    }
}