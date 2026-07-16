#pragma once

#include "dsp/delay_line.h"
#include "dsp/filter/one_pole_filter.h"

namespace ml::dsp::filter {
    class SchroederAllPass : public FractionalDelayLine
    {
    public:
        void setGain(float gain) noexcept
        {
            gain_ = gain;
        }

        [[nodiscard]] float process(float input) noexcept
        {
            const float delayed = read();
            const float w = input - gain_ * delayed;
            write(w);
            const float output = w * gain_ + delayed;
            return output;
        }

    private:
        float gain_ = 0.7f;
    };

    class CombFilter : public FractionalDelayLine
    {
    public:
        void setFeedback(float feedback) noexcept
        {
            feedback_ = feedback;
        }

        [[nodiscard]] float process(float input) noexcept
        {

            const float output = read();
            write(input + output * feedback_);
            return output;
        }

    private:
        float feedback_ = 0;
    };

    class LowpassFeedbackCombFilter : public FractionalDelayLine
    {
    public:
        void setFeedback(float feedback) noexcept
        {
            feedback_ = feedback;
        }

        void setDamp(float damp) noexcept
        {
            lowpassFilter_.setCoefficient(damp);
        }

        [[nodiscard]] float process(float input) noexcept
        {

            const float output = lowpassFilter_.process(read());
            write(input + output * feedback_);
            return output;
        }

    private:
        float feedback_ = 0;
        filter::OnePoleFilter lowpassFilter_;
    };
}