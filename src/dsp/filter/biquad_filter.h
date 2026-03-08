#pragma once

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
    class BiquadFilter
    {
    public:
        using Coefficients = BiquadCoefficients<T>;

        template <typename V>
        V process(const Coefficients& coefficients, V sample) noexcept
        {
            static_assert(std::is_convertible_v<V, T> && std::is_convertible_v<T, V>, "T and V must be convertible to each other");
            return static_cast<V>(coefficients.process(static_cast<T>(sample), x1_, x2_, y1_, y2_));
        }

        template <typename V>
        void process(const Coefficients& coefficients, V* samples, const std::size_t nSamples) noexcept
        {
            for (auto i{0u}; i < nSamples; ++i) {
                samples[i] = processSample(coefficients, samples[i]);
            }
        }

    private:
        T x1_{};
        T x2_{};
        T y1_{};
        T y2_{};
    };
}