#pragma once

#include "effect_base.h"
#include "dsp/parameter/parameter_view.h"
#include "dsp/dsp.h"

namespace ml::dsp::effects {
    class PannerEffect final : public EffectBase
    {
    public:
        PannerEffect();
        void prepare(float sampleRate) override;

    protected:
        void processInner(float *const *data, const unsigned int nFrames) noexcept override;

    private:
        parameter::ParameterView<float> panParam_;
        dsp::FloatSmoother pan_;
    };
}