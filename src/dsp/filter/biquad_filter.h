#pragma once

#include <array>
#include <optional>

#include "biquad_coefficients.h"

namespace dsp::filter {
    enum class FilterType
    {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        LowShelf,
        HighShelf,
        Peaking,
    };

    template <typename T>
    struct BiquadState
    {
        T x1_{};
        T x2_{};
        T y1_{};
        T y2_{};
    };

    template <typename T, typename V>
    T compute(const V sample, BiquadState<T> &state, const BiquadCoefficients<T> &coefficients)
    {
        static_assert(std::is_convertible_v<V, T> && std::is_convertible_v<T, V>, "T and V must be convertible to each other");
        auto &[x1, x2, y1, y2] = state;
        const auto x0 = static_cast<T>(sample);
        return coefficients.process(x0, x1, x2, y1, y2);
    }

    namespace FilterDefaults {
        constexpr double kDefaultFrequency = 440.0;
        constexpr double kDefaultQ = 0.707;
        constexpr double kDefaultBw = 1.0;
        constexpr double kDefaultSlope = 1.0;
        constexpr double kDefaultGain = 1.0;
    };

    template <typename T, std::size_t N>
    class BiquadFilter
    {
    public:
        using DataType = T;
        using Coefficients = BiquadCoefficients<T>;
        static constexpr auto kChannelCount = N;

        void setParameters(const FilterType filterType, T sampleRate, T frequencyHz, std::optional<T> q = std::nullopt, std::optional<T> bw = std::nullopt, std::optional<T> slope = std::nullopt, std::optional<T> gain = std::nullopt) noexcept
        {
            type_ = filterType;
            f_ = normalizeFrequency(sampleRate, frequencyHz);
            q_ = q.value_or(FilterDefaults::kDefaultQ);
            bw_ = bw.value_or(FilterDefaults::kDefaultBw);
            slope_ = slope.value_or(FilterDefaults::kDefaultSlope);
            gain_ = gain.value_or(FilterDefaults::kDefaultGain);
            update();
        }

        void setFrequency(T sampleRate, T frequencyHz) noexcept
        {
            f_ = normalizeFrequency(sampleRate, frequencyHz);
            update();
        }

        void update() noexcept
        {
            switch (type_) {
                case FilterType::LowPass: {
                    coefficients_ = Coefficients::lowPass(f_, q_);
                    break;
                }
                case FilterType::HighPass: {
                    coefficients_ = Coefficients::highPass(f_, q_);
                    break;
                }
                case FilterType::BandPass: {
                    coefficients_ = Coefficients::bandPass(f_, q_);
                    break;
                }
                case FilterType::Notch: {
                    coefficients_ = Coefficients::notch(f_, bw_);
                    break;
                }
                case FilterType::LowShelf: {
                    coefficients_ = Coefficients::lowShelf(f_, slope_, gain_);
                    break;
                }
                case FilterType::HighShelf: {
                    coefficients_ = Coefficients::highShelf(f_, slope_, gain_);
                    break;
                }
                case FilterType::Peaking: {
                    coefficients_ = Coefficients::peaking(f_, q_, gain_);
                    break;
                }
            }
        }

        template <typename V>
        void operator()(V *const *data, const std::size_t nFrames) noexcept
        {
            for (auto channel{0u}; channel < kChannelCount; ++channel) {
                auto &channelState = state_[channel];
                V *buffer = data[channel];
                for (auto i{0u}; i < nFrames; ++i) {
                    buffer[i] = compute(buffer[i], channelState, coefficients_);
                }
            }
        }

    private:
        FilterType type_{};
        T f_{};
        T q_{};
        T bw_{};
        T slope_{};
        T gain_{};

        Coefficients coefficients_;
        std::array<BiquadState<DataType>, kChannelCount> state_{};
    };
}