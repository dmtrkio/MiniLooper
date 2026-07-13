#include "gain_effect.h"

namespace ml::dsp::effects {
    GainEffect::GainEffect() : EffectBase("Gain")
    {
        std::vector<ParamTree> params = {
            ParamTree{Param::makeFloat("GainDb", 0.0f, dsp::Range{-60.0f, 12.0f})},
        };

        attachParameters(params);

        gainDbParam_.referTo(params[0].asParameterUnsafe());
    }

    void GainEffect::prepareInner(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        linearGain_.init(dsp::dBtoLinear(gainDbParam_.get()));
        linearGain_.setSmoothingFrames(smoothFrames);
    }

    void GainEffect::processInner(float *const *data, const unsigned int nFrames) noexcept
    {
        linearGain_.setTarget(dsp::dBtoLinear(gainDbParam_.get()));
        for (unsigned int i = 0; i < nFrames; ++i) {
            const auto gain = linearGain_();
            data[0][i] *= gain;
            data[1][i] *= gain;
        }
    }
}