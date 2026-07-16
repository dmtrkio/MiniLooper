#pragma once

#include "dsp/dsp.h"
#include "dsp/filter/ballistics_filter.h"
#include "dsp/filter/biquad_coefficients.h"
#include "dsp/filter/biquad_filter.h"
#include "dsp/parameter/parameter_view.h"
#include "effect_base.h"

namespace ml::dsp::effects {
    class AutoWah final : public EffectBase
    {
    public:
        AutoWah();

    protected:
        void prepareInner(float sampleRate) override;
        void processInner(float *const *data, unsigned int nFrames) noexcept override;

    private:
        struct Filter
        {
            using FloatType = double;
            using Coefficients = filter::BiquadCoefficients<FloatType>;
            using State = filter::BiquadState<FloatType>;

            Coefficients coef;
            State state[2];

            void process(float& left, float& right, float q, float f) noexcept
            {
                coef = Coefficients::bandPass(f, q);
                left = filter::compute(left, state[0], coef);
                right = filter::compute(right, state[1], coef);
            }
        };

        float sampleRate_ = 44100.0f;
        Filter filter_;
        filter::BallisticsFilter filterEnvelope_;

        parameter::FloatParameterView sensetivityParam_;
        FloatSmoother sensetivity_;

        parameter::FloatParameterView driveParam_;
        FloatSmoother drive_;

        parameter::FloatParameterView qParam_;
        FloatSmoother q_;
    };
}