#pragma once

#include <cmath>
#include <numbers>

namespace ml::dsp {
    class Oscillator
    {
    public:
        Oscillator() = default;

        explicit Oscillator(float frequency, float sampleRate)
        {
            updatePhaseIncr(frequency, sampleRate);
        }

        void setFrequency(float frequency, float sampleRate)
        {
            updatePhaseIncr(frequency, sampleRate);
        }

        [[nodiscard]] float sine() const
        {
            return std::sin(phase_);
        }

        [[nodiscard]] float triangle() const
        {
            return 4.0f * std::abs(phase_ * kFrac1TwoPi - 0.5f) - 1.0f;
        }

        [[nodiscard]] float saw() const
        {
            return (phase_ - kPi) * kFrac1Pi;
        }

        [[nodiscard]] float polyBlepSaw() const
        {
            const float t = phase_ * kFrac1TwoPi;
            const float dt = phaseIncr_ * kFrac1TwoPi;
            const float out = 2.0f * t - 1.0f;
            return out - polyBlep(t, dt);
        }

        [[nodiscard]] float pulse(float pulseWidth) const
        {
            return (phase_ < pulseWidth * kTwoPi) ? 1.0f : -1.0f;
        }

        [[nodiscard]] float polyBlepPulse(float pulseWidth) const
        {
            const float t = phase_ * kFrac1TwoPi;
            const float dt = phaseIncr_ * kFrac1TwoPi;

            float out = (t < pulseWidth) ? 1.0f : -1.0f;

            out += polyBlep(t, dt);

            float t2 = t - pulseWidth + 1.0f;
            t2 -= std::floor(t2);
            out -= polyBlep(t2, dt);

            return out;
        }

        void tick()
        {
            phase_ += phaseIncr_;
            phase_ -= (phase_ >= kTwoPi) * kTwoPi;
        }

        void phaseOffset(float offset)
        {
            phase_ += offset;
            phase_ -= (phase_ >= kTwoPi) * kTwoPi;
        }

        void reset()
        {
            phase_ = 0.0f;
        }

    private:
        float phase_ = 0.0f;
        float phaseIncr_ = 0.0f;

        static constexpr float kPi = std::numbers::pi_v<float>;
        static constexpr float kTwoPi = kPi * 2.0f;
        static constexpr float kFrac1Pi = 1.0f / kPi;
        static constexpr float kFrac1TwoPi = 1.0f / kTwoPi;

        void updatePhaseIncr(float frequency, float sampleRate)
        {
            const float inverseSr = 1.0f / sampleRate;
            phaseIncr_ = (kTwoPi * frequency) * inverseSr;
        }

        [[nodiscard]] static float polyBlep(float t, float dt)
        {
            if (t < dt) {
                const float x = t / dt;
                return x + x - x * x - 1.0f;
            }
            if (t > 1.0f - dt) {
                const float x = (t - 1.0f) / dt;
                return x * x + x + x + 1.0f;
            }
            return 0.0f;
        }
    };
}