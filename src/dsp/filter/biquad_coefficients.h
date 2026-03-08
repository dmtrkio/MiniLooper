#pragma once

#include <cassert>
#include <cmath>
#include <numbers>

namespace dsp::filter {
    template<typename T>
    struct BiquadCoefficients
    {
        static_assert(std::is_floating_point_v<T>, "T must be floating point type");

        T b0, b1, b2;
        T a1, a2;

        [[nodiscard]] T process(T x0, T &x1, T &x2, T &y1, T &y2) const noexcept
        {
            const T y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = x0;
            y2 = y1;
            y1 = y0;
            return y0;
        }

        static BiquadCoefficients lowPass(T cf, T q)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = sine / (2 * q);
            const T omc = 1 - cosine;

            BiquadCoefficients x;
            x.b0 = omc / 2;
            x.b1 = omc;
            x.b2 = x.b0;
            x.a1 = -2 * cosine;
            x.a2 = 1 - alpha;
            const T a0 = 1 + alpha;
            return normalize(x, a0);
        }

        static BiquadCoefficients highPass(T cf, T q)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = cf / (2 * q);
            const T opc = 1 + cosine;

            BiquadCoefficients x;
            x.b0 = opc / 2;
            x.b1 = -opc;
            x.b2 = x.b0;
            x.a1 = -2 * cosine;
            x.a2 = 1 - alpha;
            const T a0 = 1 + alpha;
            return normalize(x, a0);
        }

        static BiquadCoefficients bandPass(T cf, T bw, T q)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = sine * std::sinh((std::log(2) / 2) * bw * (cf / sine));

            BiquadCoefficients x;
            x.b0 = alpha * q;
            x.b1 = 0;
            x.b2 = -x.b0;
            x.a1 = -2 * cosine;
            x.a2 = 1 - alpha;
            const T a0 = 1 + alpha;
            return normalize(x, a0);
        }

        static BiquadCoefficients notch(T cf, T bw, T q)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = sine * std::sinh((std::log(2) / 2) * bw * (cf / sine));

            BiquadCoefficients x;
            x.b0 = 1;
            x.b1 = -2 * cosine;
            x.b2 = -x.b0;
            x.a1 = x.b1;
            x.a2 = 1 - alpha;
            const T a0 = 1 + alpha;
            return normalize(x, a0);
        }

        static BiquadCoefficients lowShelf(T cf, T slope, T gain)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = (sine / 2) * std::sqrt((gain + (1 / gain)) * (1 / slope - 1) + 2);

            const T ap = gain + 1;
            const T am = gain - 1;
            const T tmp = 2 * std::sqrt(gain) * alpha;

            BiquadCoefficients x;
            x.b0 = gain * (ap - am * cosine + tmp);
            x.b1 = 2 * gain * (am - ap * cosine);
            x.b2 = gain * (ap - am * cosine - tmp);
            x.a1 = -2 * (am + ap * cosine);
            x.a2 = ap * am * cosine - tmp;
            const T a0 = ap + am * cosine + tmp;
            return normalize(x, a0);
        }

        static BiquadCoefficients highShelf(T cf, T slope, T gain)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = (sine / 2) * std::sqrt((gain + (1 / gain)) * (1 / slope - 1) + 2);

            const T ap = gain + 1;
            const T am = gain - 1;
            const T tmp = 2 * std::sqrt(gain) * alpha;

            BiquadCoefficients x;
            x.b0 = gain * (ap + am * cosine + tmp);
            x.b1 = -2 * gain * (am + ap * cosine);
            x.b2 = gain * (ap + am * cosine - tmp);
            x.a1 = -2 * (am - ap * cosine);
            x.a2 = ap - am * cosine - tmp;
            const T a0 = ap - am * cosine + tmp;
            return normalize(x, a0);
        }

        static BiquadCoefficients peaking(T cf, T q, T gain)
        {
            const T sine = std::sin(cf);
            const T cosine = std::cos(cf);
            const T alpha = sine * std::sinh((std::log(2) / 2) * q * (cf / sine));

            BiquadCoefficients x;
            x.b0 = 1 + alpha * gain;
            x.b1 = -2 * cosine;
            x.b2 = 1 - alpha * gain;
            x.a1 = x.b1;
            x.a2 = 1 - alpha / gain;
            const T a0 = 1 + alpha / gain;
            return normalize(x, a0);
        }

        static BiquadCoefficients normalize(BiquadCoefficients x, T a0)
        {
            assert(a0 != 0);
            const auto scale = 1 / a0;
            x.b0 *= scale;
            x.b1 *= scale;
            x.b2 *= scale;
            x.a1 *= scale;
            x.a2 *= scale;
            return x;
        }

        static T normalizeFrequency(T samplingFrequency, T f)
        {
            assert(samplingFrequency != 0);
            static constexpr T kTwoPi = std::numbers::pi_v<T> * 2;
            return kTwoPi * f / samplingFrequency;
        }
    };
}
