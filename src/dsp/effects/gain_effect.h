#pragma once

#include "effect_base.h"
#include "dsp/parameter/parameter_view.h"
#include "dsp/dsp.h"

namespace ml::dsp::effects {
    class GainEffect final : public EffectBase
    {
    public:
        GainEffect();

    protected:
        void prepareInner(float sampleRate) override;
        void processInner(float *const *data, const unsigned int nFrames) noexcept override;

    private:
        parameter::ParameterView<float> gainDbParam_;
        dsp::FloatSmoother linearGain_;
    };
}