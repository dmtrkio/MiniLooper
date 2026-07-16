#pragma once

namespace ml::dsp::filter {
    class OnePoleFilter
    {
    public:
        explicit OnePoleFilter(float coeff = 0.0f)
            : coeff_(coeff)
        {}

        void setCoefficient(float coeff) noexcept
        {
            coeff_ = coeff;
        }

        [[nodiscard]] float process(float input) noexcept
        {
            const float output = input + coeff_ * (prevOutput_ - input);
            prevOutput_ = output;
            return output;
        }

    private:
        float coeff_ = 0.0f;
        float prevOutput_ = 0.0f;
    };
}